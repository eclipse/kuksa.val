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





template<typename T>
void checkNumTypes(jsoncons::json &meta, jsoncons::json &val )
{
    //Double can be larger than largest allowed VSS type ((u)int64)
    double dval = val.as<long double>();

    if (dval < std::numeric_limits<T>::min() || dval > std::numeric_limits<T>::max() ) {
        std::stringstream msg;
        msg << "Value " << dval << "is out of bounds for type " << meta["datatype"].as<std::string>();
        throw outOfBoundException(msg.str());
    }

    if ( meta.contains("min") && dval < meta["min"].as<double>() ) {
        std::stringstream msg;
        msg << "Value " << dval << "is out of bounds. Allowed minimum is " <<  meta["min"].as<double>();
        throw outOfBoundException(msg.str());
    }

    if ( meta.contains("max") && dval > meta["max"].as<double>() ) {
        std::stringstream msg;
        msg << "Value " << dval << "is out of bounds. Allowed maximum is " <<  meta["max"].as<double>();
        throw outOfBoundException(msg.str());
    }
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
        //o nothing. If it was a valid JSON it can always be string
    }
    else {
      std::string msg = "The datatype " + dt + " is not supported ";
      throw genException(msg);
    }

}
