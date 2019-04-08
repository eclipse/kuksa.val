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

using namespace std;

// No path on Tree exception
class noPathFoundonTree: public exception
{
  private:
    string message; 

  public:
    
    noPathFoundonTree(string path) {
        message = "Path " + path + " not found on the VSS Tree";
    }

  virtual const char* what() const throw()
  {
    return message.c_str();
  }
} ;

// Generic exception
class genException: public exception
{
  private:
    string message;

  public:
    
    genException(string msg) {
       message = msg;
    }

  virtual const char* what() const throw()
  {
    return message.c_str();
  }
} ;

// not permitted- no permission
class noPermissionException: public exception
{
  private:
    string message;

  public:
    
    noPermissionException(string msg) {
       message = msg;
    }

  virtual const char* what() const throw()
  {
    return message.c_str();
  }
} ;

// value out of bound exception
class outOfBoundException: public exception
{
  private:
    string message;

  public:
    
    outOfBoundException(string msg) {
       message = msg;
    }

  virtual const char* what() const throw()
  {
    return message.c_str();
  }
} ;

#endif

