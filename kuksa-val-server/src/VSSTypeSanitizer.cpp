/*
 * ******************************************************************************
 * Copyright (c) 2020 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH
 * *****************************************************************************
 */


#include "VssDatabase.hpp"
#include "ILogger.hpp"
#include "exception.hpp"
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/exception/diagnostic_information.hpp>



/** Converting numeric types. We will rely on JSoncons implementation, i.e. if it is 
 *  an acceptable number for jsoncons, so it is for us.
 *  See https://github.com/danielaparker/jsoncons/blob/master/include/jsoncons/detail/parse_number.hpp
 *  for what they are doing.
 *  Jsoncons will report out-of bounds exceptions for int types
 *  For float types we do reject +/-infinity, as we assume setting this
 *  are not the intention of a client
 */
template<typename T>
void checkNumTypes(jsoncons::json &meta, jsoncons::json &val )
{
    T cval;
    try {
        cval = val.as<T>();
    }
    catch(std::exception const& e) {
      std::stringstream msg;
      msg << "Value " << val << " can not be converted to defined type " << meta["datatype"].as_string() << ". Reason: " << e.what();
      throw outOfBoundException(msg.str());
    }
    catch(...) {
      std::stringstream msg;
      msg << "Value " << val << " can not be converted to defined type " << meta["datatype"].as_string() << ". Reason: " << boost::current_exception_diagnostic_information();
      throw outOfBoundException(msg.str());
      
    }

    if (std::numeric_limits<T>::has_infinity && (cval == std::numeric_limits<T>::infinity() || cval == -std::numeric_limits<T>::infinity()) ) {
        throw outOfBoundException("Value out of bounds. Reason: Infinity");
    }
    

    if ( meta.contains("min") && cval < meta["min"].as<T>() ) {
        std::stringstream msg;
        msg << "Value " << cval << " is out of bounds. Allowed minimum is " <<  meta["min"].as<double>();
        throw outOfBoundException(msg.str());
    }

    if ( meta.contains("max") && cval > meta["max"].as<T>() ) {
        std::stringstream msg;
        msg << "Value " << cval << " is out of bounds. Allowed maximum is " <<  meta["max"].as<double>();
        throw outOfBoundException(msg.str());
    }
    val=val.as<T>();
}

void checkBoolType(jsoncons::json &val ) {
    std::string v=val.as<std::string>();
    boost::algorithm::erase_all(v, "\"");

    if ( v == "true") {
        val=true;
    }
    else if ( v == "false" ) {
        val=false;
    }
    else {
        std::stringstream msg;
        msg << val.as_string() << " is not a bool. Valid values are true and false ";
        throw outOfBoundException(msg.str());
    }
}

void checkEnumType(jsoncons::json &enumDefinition, jsoncons::json &val ) {
    
    for (const auto& item: enumDefinition.array_range()){
      if(item.as_string() == val){
        return;
      }
    }

    std::stringstream msg;
    msg << val.as_string() << " is a defined enum value. Valid values are " << print(enumDefinition);
    throw outOfBoundException(msg.str());
}

void VssDatabase::checkArrayType(std::string& subdatatype, jsoncons::json &val) {
    jsoncons::json metadata;
    metadata["datatype"] = subdatatype;
    try{
        for (auto v : val.array_range())
        {
            checkAndSanitizeType(metadata, v);
        }
    }
    catch(std::exception const& e) {
      std::stringstream msg;
      msg << "Value " << val << " can not be converted to defined type " << subdatatype << "[]. Reason: " << e.what();
      throw outOfBoundException(msg.str());
    } catch(...) {
      std::stringstream msg;
      msg << "Value " << val << " can not be converted to defined type " << subdatatype << "[]. Reason: " << boost::current_exception_diagnostic_information();
      throw outOfBoundException(msg.str());
    }
}


/** This will check whether &val val is a valid value for the sensor described
 *  by meta  and whether  it is within the limits defined by VSS if any */
void VssDatabase::checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val) {
    std::string dt=meta["datatype"].as<std::string>();
    if (dt == "uint8") {
        checkNumTypes<uint8_t>(meta,val);
    }
    else if (dt == "int8") {
        checkNumTypes<int8_t>(meta,val);
    }
    else if (dt == "uint16") {
        checkNumTypes<uint16_t>(meta,val);
    }
    else if (dt == "int16") {
        checkNumTypes<int16_t>(meta,val);
    }
    else if (dt == "uint32") {
        checkNumTypes<uint32_t>(meta,val);
    }
    else if (dt == "int32") {
        checkNumTypes<int32_t>(meta,val);
    }
    else if (dt == "uint64") {
        checkNumTypes<uint64_t>(meta,val);
    }
    else if (dt == "int64") {
        checkNumTypes<int64_t>(meta,val);
    }
    else if (dt == "float") {
        checkNumTypes<float>(meta,val);
    }
    else if (dt == "double") {
        checkNumTypes<double>(meta,val);
    }
    else if (dt == "boolean") { 
        checkBoolType(val);
    }
    else if (dt == "string") { 
      if (meta.contains("enum") ) {
        checkEnumType(meta["enum"], val);
      }
    }
    else if (dt.rfind("[]") == dt.size()-2){
        std::string subdatatype = dt.substr(0, dt.size()-2);
        checkArrayType(subdatatype, val);
    }
    else {
      std::string msg = "The datatype " + dt + " is not supported ";
      throw genException(msg);
    }

}
