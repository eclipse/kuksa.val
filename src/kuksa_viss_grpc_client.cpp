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
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "kuksa.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using kuksa::viss_client;
using kuksa::CommandRequest;
using kuksa::CommandReply;

class GrpcConnection {
 public:
  GrpcConnection(std::shared_ptr<Channel> channel)
      : stub_(viss_client::NewStub(channel)) {}
  // get the command from the terminal and request the server
  std::string HandleRequest(){
    // Container for the data we send to the server.
    CommandRequest request;
    // Container for the data we expect from the server.
    CommandReply reply;
    std::string command;
    // Get command input from input
    std::getline(std::cin,command);
    request.set_command(command);
    ClientContext context;
    // The actual RPC
    Status status = stub_->HandleRequest(&context, request, &reply);
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }
 private:
  std::unique_ptr<viss_client::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50051";
  }
  GrpcConnection connGrpcSes(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string reply;
  std::string abort = "quit";
  // prepare for getting a command while command is not quit
  while(abort.compare(reply) != 0){
    std::cout << "Test-Client>";
    reply = connGrpcSes.HandleRequest();
    std::cout << reply << std::endl;
  }

  return 0;
}
