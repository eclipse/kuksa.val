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
#include "wsserver.hpp"
#include "vsscommandprocessor.hpp"
#include "visconf.hpp"

#define PORT 8090
 

uint16_t connections[MAX_CLIENTS + 1] = {0};
wsserver* wserver;

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



wsserver::wsserver(int port, bool secure) {
  isSecure = secure;

  if(isSecure) {
     secureServer = new WssServer("Server.pem", "Server.key");
     secureServer->config.port = port;
  } else {
     insecureServer = new WsServer();
     insecureServer->config.port = port;
  }
  
  tokenValidator =  new authenticator("appstacle", "HS256");
  subHandler = new subscriptionhandler(this, tokenValidator);
  database = new vssdatabase(subHandler);
  cmdProcessor = new vsscommandprocessor(database, tokenValidator, subHandler);
  wserver = this;
}

wsserver::~wsserver() {
   // destructor
  
}


static void onMessage (shared_ptr<WssServer::Connection> connection, string message) {
  #ifdef DEBUG
      cout << "Server: Message received: \"" << message << "\" from " << connection->remote_endpoint_address() << endl;
  #endif
      string response = wserver->cmdProcessor->processQuery(message, connection->channel);
     
      auto send_stream = make_shared<WssServer::SendStream>();
      *send_stream << response;
      connection->send(send_stream);
}


static void onMessage (shared_ptr<WsServer::Connection> connection, string message) {
  #ifdef DEBUG
      cout << "Server: Message received: \"" << message << "\" from " << connection->remote_endpoint_address() << endl;
  #endif
      string response = wserver->cmdProcessor->processQuery(message, connection->channel);
     
      auto send_stream = make_shared<WsServer::SendStream>();
      *send_stream << response;
      connection->send(send_stream);
}


void wsserver::startServer(string endpointName) {
   if( isSecure ) {
   cout<<"starting secure WS server"<<endl;
           
    auto &vssEndpoint = secureServer->endpoint["^/vss/?$"];

   vssEndpoint.on_message = [](shared_ptr<WssServer::Connection> connection, shared_ptr<WssServer::Message> message) {
      auto message_str = message->string();
      onMessage(connection , message_str);

    };

   vssEndpoint.on_open = [](shared_ptr<WssServer::Connection> connection) {
       connection->channel.setConnID(generateConnID());
       cout << "Server: Opened connection " << connection->remote_endpoint_address()<< "conn ID " << connection->channel.getConnID() << endl;
    };

   // See RFC 6455 7.4.1. for status codes
   vssEndpoint.on_close = [](shared_ptr<WssServer::Connection> connection, int status, const string & /*reason*/) {
        uint32_t clientID = connection->channel.getConnID()/CLIENT_MASK;
        connections[clientID] = 0;
        //removeAllSubscriptions(clientID);
        cout << "Server: Closed connection " << connection->remote_endpoint_address() << " with status code " << status << endl;
    };

   // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
   vssEndpoint.on_error = [](shared_ptr<WssServer::Connection> connection, const SimpleWeb::error_code &ec) {
        uint32_t clientID = connection->channel.getConnID()/CLIENT_MASK;
        connections[clientID] = 0;
        //removeAllSubscriptions(clientID);
        cout << "Server: Error in connection " << connection->remote_endpoint_address()<<" with con ID "<< connection->channel.getConnID()<< ". "
         << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

      cout << "started Secure WS server" << endl; 
      secureServer->start();
   } else {

     cout<<"starting insecure WS server"<<endl;
  
           
   auto &vssEndpoint = insecureServer->endpoint["^/vss/?$"];

   vssEndpoint.on_message = [](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
      auto message_str = message->string();
      onMessage(connection , message_str);

    };

   vssEndpoint.on_open = [](shared_ptr<WsServer::Connection> connection) {
       connection->channel.setConnID(generateConnID());
       cout << "Server: Opened connection " << connection->remote_endpoint_address()<< "conn ID " << connection->channel.getConnID() << endl;
    };

   // See RFC 6455 7.4.1. for status codes
   vssEndpoint.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        uint32_t clientID = connection->channel.getConnID()/CLIENT_MASK;
        connections[clientID] = 0;
        //removeAllSubscriptions(clientID);
        cout << "Server: Closed connection " << connection->remote_endpoint_address() << " with status code " << status << endl;
    };

   // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
   vssEndpoint.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code &ec) {
        uint32_t clientID = connection->channel.getConnID()/CLIENT_MASK;
        connections[clientID] = 0;
        //removeAllSubscriptions(clientID);
        cout << "Server: Error in connection " << connection->remote_endpoint_address()<<" with con ID "<< connection->channel.getConnID()<< ". "
         << "Error: " << ec << ", error message: " << ec.message() << endl;
    };

      cout << "started insecure WS server" << endl; 
      insecureServer->start();

   
   }
   
}

void wsserver::sendToConnection(uint32_t connectionID, string message) {
    if( isSecure) {
    auto send_stream = make_shared<WssServer::SendStream>();
        *send_stream << message;
        for(auto &a_connection : secureServer->get_connections()) {
          if(a_connection->channel.getConnID() == connectionID){
             a_connection->send(send_stream);
             return;
          }
      }
    }else {
     auto send_stream = make_shared<WsServer::SendStream>();
        *send_stream << message;
        for(auto &a_connection : insecureServer->get_connections()) {
          if(a_connection->channel.getConnID() == connectionID){
             a_connection->send(send_stream);
             return;
          }
      }



    }
}


void* startWSServer(void * arg) {
   
   wserver->startServer("");

}

/**
 * @brief  Test main.
 * @return
 */
int main(int argc, char* argv[])
{
 
        wsserver server(PORT, false);
        server.database->initJsonTree();
        pthread_t startWSServer_thread;
        
        
        /* create the web socket server thread. */
        if(pthread_create(&startWSServer_thread, NULL, &startWSServer, NULL )) {
         cout << "Error creating websocket server run thread"<<endl;
         return 1;

        }

      getchar();
}


