/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH and others.
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
#include <string>
#include <iostream>
#include <unistd.h>

#include "vss.hpp"
#include "vssserializer.hpp"
#include "vsscommandprocessor.hpp"
#include "vssdatabase.hpp"
#include "visconf.hpp"
#include "server_wss.hpp"
#include <json.hpp>


using namespace std;

int connectionHandle = -1;

using WssServer = SimpleWeb::SocketServer<SimpleWeb::WSS>;


uint16_t connections[MAX_CLIENTS + 1] = {0};

std::string query;
WssServer server("Server.pem", "Server.key");


struct node mainNode;
struct signal* ptrToSignals[MAX_SIGNALS];
UInt32 subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];


uint32_t generateConnID() {
 uint32_t  retValueValue = 0;
   for(int i=1 ; i<(MAX_CLIENTS + 1) ; i++) {
     if(connections[i] == 0) {
         connections[i]= i;
         retValueValue = CLIENT_MASK * (i);
         return retValueValue;
     } 
  }
    return retValueValue;
}



// Thread that starts the WS server.
void* startWSServer(void * arg) {

   cout<<"starting secure WS server"<<endl;
           
   server.config.port = 8090;

   auto &vssEndpoint = server.endpoint["^/vss/?$"];

   vssEndpoint.on_message = [](shared_ptr<WssServer::Connection> connection, shared_ptr<WssServer::Message> message) {
      auto message_str = message->string();
#ifdef DEBUG
      cout << "Server: Message received: \"" << message_str << "\" from " << connection->remote_endpoint_address() << endl;
#endif
      string response = processQuery(message_str, connection->connectionID );
     
      auto send_stream = make_shared<WssServer::SendStream>();
      *send_stream << response;
      connection->send(send_stream);
  };

      vssEndpoint.on_open = [](shared_ptr<WssServer::Connection> connection) {
         connection->connectionID = generateConnID();
         cout << "Server: Opened connection " << connection->remote_endpoint_address()<< "conn ID " << connection->connectionID << endl;
    };

      // See RFC 6455 7.4.1. for status codes
      vssEndpoint.on_close = [](shared_ptr<WssServer::Connection> connection, int status, const string & /*reason*/) {
         UInt32 clientID = connection->connectionID/CLIENT_MASK;
         connections[clientID] = 0;
         removeAllSubscriptions(clientID);
         cout << "Server: Closed connection " << connection->remote_endpoint_address() << " with status code " << status << endl;
    };

  // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    vssEndpoint.on_error = [](shared_ptr<WssServer::Connection> connection, const SimpleWeb::error_code &ec) {
        UInt32 clientID = connection->connectionID/CLIENT_MASK;
        connections[clientID] = 0;
        removeAllSubscriptions(clientID);
        cout << "Server: Error in connection " << connection->remote_endpoint_address()<<" with con ID "<< connection->connectionID<< ". "
         << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

      cout << "started Secure WS server" << endl; 
      server.start();
}


/**
 * @brief  Test main.
 * @return
 */
int main(int argc, char* argv[])
{
 
        memset(subscribeHandle, 0, sizeof(subscribeHandle));
	//Init new data structure
	initJsonTree();
	initVSS (&mainNode , ptrToSignals);
   
        pthread_t startWSServer_thread;
        
        /* create the web socket server thread. */
        if(pthread_create(&startWSServer_thread, NULL, &startWSServer, NULL )) {

         cout << "Error creating websocket server run thread"<<endl;
         return 1;

        }

      getchar();
}



