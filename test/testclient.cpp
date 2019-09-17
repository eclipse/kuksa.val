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

#include "client_wss.hpp"
#include <json.hpp>

using namespace std;
using namespace jsoncons;

using WssClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;

string url = "localhost:8090/vss";

bool subThread = false;


string getRequest(string path ) {

  json req;
   req["requestId"] = 1234;
   req["action"]= "get";
   req["path"] = string(path);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();

}

string setRequest(string path, int val) {

  json req;
   req["requestId"] = 1235;
   req["action"]= "set";
   req["path"] = string(path);
   req["value"] = val;
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();

}

string getMetarequest( string path ) {

  json req;
   req["requestId"] = 1236;
   req["action"]= "getMetadata";
   req["path"] = string(path);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();
}

string getSubRequest (string path) {

   json req;
   req["requestId"] = 1237;
   req["action"]= "subscribe";
   req["path"] = string(path);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();

}

string getAuthRequest(string token) {

  json req;
   req["requestId"] = 1238;
   req["action"]= "authorize";
   req["tokens"] = string(token);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();

}

string getKuksaAuthRequest(string clientID , string secret) {

  json req;
   req["requestId"] = 1238;
   req["action"]= "kuksa-authorize";
   req["clientid"] = clientID;
   req["secret"] = secret;
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();

}

void sendRequest(shared_ptr<WssClient::Connection> connection) {

    string path, function;
    cout << "Enter vss path eg : Vehicle.OBD.EngineSpeed " <<endl;
    getline (cin, path);
    cout << "Enter vis Function eg: authorize, kuksa-authorize, get, set, getmetadata "<< endl;
    getline (cin, function);
    string command;
    if(function == "get") {
       command = getRequest(path);
    } else if (function == "set") {
       string val;
       cout << "Enter an integer value for the path "<< endl;
       getline (cin, val);
       int value = atoi(val.c_str());
       command = setRequest(path, value);
    } else if (function == "getmetadata") {
        command = getMetarequest(path);
    } else if (function == "authorize") {
        string token;
        cout << "Enter Token "<< endl;
        getline (cin, token);
        command = getAuthRequest(token);
    } else if (function == "kuksa-authorize") {
        string clientid, secret;
        cout << "Enter clientid <SPACE> secret "<< endl;
        cin >> clientid >> secret;
        command = getKuksaAuthRequest(clientid, secret);
    }
    
    auto send_stream = make_shared<WssClient::SendStream>();
    *send_stream << command;
    connection->send(send_stream);

}


void startWSClient() {

  WssClient client(url , true ,"Client.pem", "Client.key","CA.pem");

  client.on_message = [](shared_ptr<WssClient::Connection> connection, shared_ptr<WssClient::Message> message) {
    cout << "Response >> " << message->string() << endl;
    sendRequest(connection); 
  };

  client.on_open = [](shared_ptr<WssClient::Connection> connection) {
    cout << "Connection with server at " << url << " opened" << endl;
    sendRequest(connection); 
  };

  client.on_close = [](shared_ptr<WssClient::Connection> /*connection*/, int status, const string & /*reason*/) {
    cout << "Connection closed with status code " << status << endl;
  };

  // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  client.on_error = [](shared_ptr<WssClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
    cout << "Error: " << ec << ", message: " << ec.message() << endl;
  };

client.start();
}


int main() {

   startWSClient();

}
