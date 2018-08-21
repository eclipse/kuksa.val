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
#include <exception>
#include <string>

using namespace std;

// No path on Tree exception
class noPathFoundonTree: public exception
{
  private:
    string loc;

  public:
    
    noPathFoundonTree(string path) {
        loc = path;
    }

  virtual const char* what() const throw()
  {
    string message = "Path " + loc + " not found on the VSS Tree";
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


