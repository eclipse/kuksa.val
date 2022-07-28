/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */
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
