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

string url = "localhost:9000/vss";


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

void sendRequest(shared_ptr<WssClient::Connection> connection) {

    string path, function;
    cout << "Enter vss path eg : Signal.Drivetrain.InternalCombustionEngine.RPM " <<endl;
    getline (cin, path);
    cout << "Enter vis Function eg: get, set "<< endl;
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
    }
    
    auto send_stream = make_shared<WssClient::SendStream>();
    *send_stream << command;
    connection->send(send_stream);

}


void startWSClient() {

  WssClient client(url , false);

  client.on_message = [](shared_ptr<WssClient::Connection> connection, shared_ptr<WssClient::Message> message) {
    cout << "Response >> " << message->string() << endl;
    sendRequest(connection); 
  };

  client.on_open = [](shared_ptr<WssClient::Connection> connection) {
    cout << "Connection wirh server at " << url << " opened" << endl;
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
