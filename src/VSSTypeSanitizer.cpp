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





template<typename T>
void checkIntTypes(std::shared_ptr<ILogger> logger, jsoncons::json &meta, jsoncons::json &val )
{
    std::cout << "Running for " << __PRETTY_FUNCTION__  << '\n';
    //Double can be larger than largest allowed vss type signed or unsingedn ((u)int64)
    double dval = val.as<double>();
    if (dval < std::numeric_limits<T>::min() || dval > std::numeric_limits<T>::max() ) {
        std::stringstream msg;
        msg << "Value " << dval << "is out of bounds for type " << meta["datatype"].as<std::string>();
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());
        throw outOfBoundException(msg.str());
    }



}

/** This will check whether &val val is a valid value for the sensor described
 *  by meta  and whether  it is within the limits defined by VSS if any */
void VssDatabase::checkAndSanitizeType(jsoncons::json &meta, jsoncons::json &val) {
    std::string dt=meta["datatype"].as<std::string>();
    if (dt == "uint8") {
        checkIntTypes<uint8_t>(this->logger_, meta,val);
    }
}


/*
[[deprecated]]
  void checkTypeAndBound(std::shared_ptr<ILogger> logger, string value_type, jsoncons::json &val) {
    bool typeValid = false;

    boost::algorithm::to_lower(value_type);

    if (value_type == "uint8") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint8_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint8_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "uint16") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint16_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint16_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "uint32") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint32_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint32_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "uint64") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<uint64_t>::max()) &&
            (longDoubleVal >= numeric_limits<uint64_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int8") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int8_t>::max()) &&
            (longDoubleVal >= numeric_limits<int8_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int16") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int16_t>::max()) &&
            (longDoubleVal >= numeric_limits<int16_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int32") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int32_t>::max()) &&
            (longDoubleVal >= numeric_limits<int32_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "int64") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      if (!((longDoubleVal <= numeric_limits<int64_t>::max()) &&
            (longDoubleVal >= numeric_limits<int64_t>::min()))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value " << val.as<float>()
            << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());
        throw outOfBoundException(msg.str());
      }
    }  else if (value_type == "float") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      float max = numeric_limits<float>::max();
      float min = numeric_limits<float>::lowest();
      if (!((longDoubleVal <= max) && (longDoubleVal >= min))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value '" << val.as<double>()
            << "' is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "double") {
      typeValid = true;
      long double longDoubleVal;
      ValidateValue(logger, val, longDoubleVal);
      double max = numeric_limits<double>::max();
      double min = numeric_limits<double>::lowest();
      if (!((longDoubleVal <= max) && (longDoubleVal >= min))) {
        std::stringstream msg;
        msg << "The type " << value_type << " with value "
            << val.as<long double>() << " is out of bound";
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str());

        throw outOfBoundException(msg.str());
      }
    } else if (value_type == "boolean") {
      string v=val.as<string>();
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
        logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg.str()); 
        std::cout << pretty_print(val) << std::endl;
        throw outOfBoundException(msg.str());
      }
      typeValid = true;
    } else if (value_type == "string") {
      typeValid = true;
    }

    if (!typeValid) {
      string msg = "The type " + value_type + " is not supported ";
      logger->Log(LogLevel::ERROR, "VssDatabase::setSignal: " + msg);

      throw genException(msg);
    }
  }

  */