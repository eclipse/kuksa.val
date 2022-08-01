/**********************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/

#ifndef __EXCEPTION_HPP__
#define __EXCEPTION_HPP__

#include <exception>
#include <string>

// No path on Tree exception
class noPathFoundonTree : public std::exception {
 private:
  std::string message;

 public:
  noPathFoundonTree(std::string path) {
    message = "Path " + path + " not found on the VSS Tree";
  }

  virtual const char* what() const throw() { return message.c_str(); }
};

// Generic exception
class genException : public std::exception {
 private:
  std::string message;

 public:
  genException(std::string msg) { message = msg; }

  virtual const char* what() const throw() { return message.c_str(); }
};

// not permitted- no permission
class noPermissionException : public std::exception {
 private:
  std::string message;

 public:
  noPermissionException(std::string msg) { message = msg; }

  virtual const char* what() const throw() { return message.c_str(); }
};

// value out of bound exception
class outOfBoundException : public std::exception {
 private:
  std::string message;

 public:
  outOfBoundException(std::string msg) { message = msg; }

  virtual const char* what() const throw() { return message.c_str(); }
};


// value out of bound exception
class notValidException : public std::exception {
 private:
  std::string message;

 public:
  notValidException(std::string msg) { message = msg; }

  virtual const char* what() const throw() { return message.c_str(); }
};

// value not set before get
class notSetException : public std::exception {
 private:
  std::string message;

 public:
  notSetException(std::string msg) { message = msg; }

  virtual const char* what() const throw() { return message.c_str(); }
};

#endif
