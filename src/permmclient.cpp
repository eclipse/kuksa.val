/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdexcept>
#include <iostream>

#include "exception.hpp"
#include "permmclient.hpp"
#include "ILogger.hpp"

using namespace std;

#define SERVER "/home/pratheek/socket/kuksa_w3c_perm_management"

json getPermToken(std::shared_ptr<ILogger> logger, string clientName, string clientSecret) {

  // Open unix socket connection.
   struct sockaddr_un addr;
   int fd;

   if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      throw genException("Unable to create a unix socket");
      //logger->Log(LogLevel::ERROR, "Unable to create a unix socket");
   }

   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
   strncpy(addr.sun_path, SERVER, sizeof(addr.sun_path)-1);


   if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      throw genException("Unable to connect to server");
      //logger->Log(LogLevel::ERROR, "Unable to connect to server");
   }


   // Create request in JSON format.
   jsoncons::json requestJson;
   requestJson["api"] = "w3c-visserver";
   requestJson["appid"] = clientName;
   requestJson["secret"] = clientSecret;
   string request = requestJson.as<string>();

   int length = request.length();
   // Send and wait for response from the permmanagent daemon.
   if(write(fd, request.c_str(), length) != length) {
     logger->Log(LogLevel::ERROR, "Request not sent completely");
   } else {
     logger->Log(LogLevel::INFO, "Request sent ");
   }

   char response_buf[1024 * 10] = {0};
   if (read(fd, response_buf, sizeof(response_buf)) == -1) {
      logger->Log(LogLevel::ERROR, "Response read from server failed!");
      throw genException("Response read from server failed!");
   }

   logger->Log(LogLevel::INFO, "Response read from server ");

   string response(response_buf);
   jsoncons::json respJson = jsoncons::json::parse(response);
   return respJson;
}

// test main
/*int main(int argc, char* argv[]) {
   string data = getPermToken("client","secret");
   cout << data << endl;
   return 0;
}*/
