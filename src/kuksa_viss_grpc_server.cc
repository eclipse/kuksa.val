/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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


#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "kuksa.grpc.pb.h"
#include "BasicLogger.hpp"
#include "VssCommandProcessor.hpp"
#include "VssDatabase.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kuksa::viss_client;
using kuksa::CommandReply;
using kuksa::CommandRequest;
using kuksa::ConnectRequest;
using kuksa::ConnectReply;

using namespace std;
using namespace boost;

// Logic and data behind the server's behavior.
class ConnectServiceImpl final : public viss_client::Service {
  Status HandleRequest(ServerContext* context, const CommandRequest* request,
                  CommandReply* reply) override {
    // Get the request from the client
    string command = request->command();
    cout << "Command:" << command << "\n";
    temp =  split(command);
    action = temp[0];
    cout << "Action:" << action << "\n";
    path = temp[1];
    cout << "Path:" << path << "\n";
    _str = "{\"action\": \"" + action + "\",\"path\": \"" + path + "\"" + "}";
    response = _str;
    cout << "Response: " << response << endl;

    return Status::OK;
  }
  private:
  std::string response;
  vector<string> temp;
  string action;
  string path;
  string _str;

  vector<string> split(string const& input, string const& separator = " ")
  {
    vector<string> result;
    string::size_type position, start = 0;
  
    while (string::npos != (position = input.find(separator, start)))
    {
      result.push_back(input.substr(start, position-start));
      start = position + separator.size();
    }
  
    result.push_back(input.substr(start));
    return result;
  }
};

void RunServer() {
  string server_address("0.0.0.0:50051");
  ConnectServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  unique_ptr<Server> server(builder.BuildAndStart());
  cout << "Kuksa viss grpc server Version 1.0.0" << endl;
  cout << "Server listening on " << server_address << endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}