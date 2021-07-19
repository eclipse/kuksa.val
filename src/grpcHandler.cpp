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
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "grpcHandler.hpp"

#include "kuksa.grpc.pb.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kuksa::viss_client;
using kuksa::CommandReply;
using kuksa::CommandRequest;

grpcHandler handler;

// Logic and data behind the server's behavior.
class RequestServiceImpl final : public viss_client::Service {
  Status HandleRequest(ServerContext* context, const CommandRequest* request,
                  CommandReply* reply) override {
    // Get the request from the client
    string command = request->command();
    temp =  split(command);
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::stringstream ss;
    ss << uuid;
    reqID = ss.str();
    if(temp.size() > 0) {
      firstarg = temp[0];
      _str = "{\"action\": \"" + firstarg + "}";
    }
    if(temp.size() > 1) {
      secarg = temp[1];
      if(firstarg == "authorize"){
        _str = "{\"action\": \"" + firstarg + "\",\"tokens\": \"" + secarg + "\",\"requestId\": \"" + reqID + "\"}";
      }
      else{
         _str = "{\"action\": \"" + firstarg + "\",\"path\": \"" + secarg + "\",\"requestId\": \"" + reqID + "\"}";
      }
    }
    if(temp.size() > 2){
      thirdarg = temp[2];
      _str = "{\"action\": \"" + firstarg + "\",\"path\": \"" + secarg + "\",\"value\": \""+ thirdarg + "\",\"requestId\": \"" + reqID + "\"}";
    }
    if(firstarg == "quit"){
      reply->set_message(firstarg);
    }
    else{
      auto Processor = handler.getGrpcProcessor();
      request_json = Processor->processQuery(_str);
      reply->set_message(request_json);
    }

    return Status::OK;
  }
  private:
  std::string request_json;
  std::string reqID;
  vector<string> temp;
  string firstarg;
  string secarg;
  string thirdarg;
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

void grpcHandler::read (const std::string& filename, std::string& data)
{
  std::ifstream file ( filename.c_str (), std::ios::in );

	if(file.is_open())
	{
		std::stringstream ss;
		ss << file.rdbuf();

		file.close();

		data = ss.str();
	}
  else{
    std::cout << "Failed to read SSL certificate" << filename << std::endl;
  }
	return;
}

void grpcHandler::RunServer(std::shared_ptr<VssCommandProcessor> Processor, std::string certPath) {
  string server_address("127.0.0.1:50051");
  RequestServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  std::string key;
	std::string cert;
  std::string root;
  std::string serverkeyfile_= certPath + "/Server.key";
  std::string serverpemfile_= certPath + "/Server.pem";
  std::string serverrootfile= certPath + "/CA.pem";
  read(serverkeyfile_, cert);
	read(serverpemfile_, key);
  read(serverrootfile, root);

  grpc::SslServerCredentialsOptions::PemKeyCertPair keycert{
    cert,
    key
  };

	grpc::SslServerCredentialsOptions sslOps;
  sslOps.pem_root_certs = root;
	sslOps.pem_key_cert_pairs.push_back(keycert);

	builder.AddListeningPort(server_address, grpc::SslServerCredentials(sslOps));

  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::shared_ptr<Server> server(builder.BuildAndStart());
  handler.grpcServer = server;
  handler.grpcProcessor = Processor;
  cout << "Kuksa viss grpc server Version 1.0.0" << endl;
  cout << "Server listening on " << server_address << endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  handler.grpcServer->Wait();
}

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

void grpcHandler::startClient(std::string port, std::string certPath){
  std::string cert; 
	std::string key;
  std::string root;
  std::string clientpemfile_ = certPath + "/Client.pem";
  std::string clientkeyfile_ = certPath + "/Client.key";
  std::string clientrootfile_ = certPath + "/CA.pem";
  read (clientpemfile_, cert);
	read (clientkeyfile_, key);
  read (clientrootfile_, root);
  grpc::SslCredentialsOptions opts; 
  opts.pem_cert_chain = cert;
	opts.pem_private_key = key;
  opts.pem_root_certs = root;
  GrpcConnection connGrpcSes(
      grpc::CreateChannel(port, grpc::SslCredentials(opts)));
  std::string reply;
  std::string abort = "quit";
  // prepare for getting a command while command is not quit
  while(abort.compare(reply) != 0){
    std::cout << "Test-Client>";
    reply = connGrpcSes.HandleRequest();
    std::cout << reply << std::endl;
  }
}

grpcHandler::grpcHandler() = default;

grpcHandler::~grpcHandler(){
    handler.grpcServer->Shutdown();
}
