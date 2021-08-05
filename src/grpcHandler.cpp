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
#include "WsChannel.hpp"

#include "kuksa.grpc.pb.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kuksa::viss_client;

grpcHandler handler;

kuksa::kuksaChannel& addClient(boost::uuids::uuid &uuid){
  auto newChannel = std::make_shared<kuksa::kuksaChannel>();
  uint64_t value = reinterpret_cast<uint64_t>(uuid.data);
  newChannel->set_connectionid(value);
  newChannel->set_typeofconnection(kuksa::kuksaChannel_Type_GRPC);
  return *newChannel;
}

// class for reading certificates
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

// Logic and data behind the servers behaviour
// implementation of the rpc interfaces server side
class RequestServiceImpl final : public viss_client::Service {                                          
  Status GetMetaData(ServerContext* context, const kuksa::commandRequest* request,
                  kuksa::data* reply) override {
                    jsoncons::json req_json;
                    req_json["action"] = "getMetaData";
                    req_json["path"] = request->path_();
                    req_json["requestId"] = request->reqid_();
                    auto Processor = handler.getGrpcProcessor();
                    auto result = Processor->processGetMetaData(req_json);
                    reply->set_value_(result);
                    return Status::OK;}

  Status AuthorizeChannel(ServerContext* context, const kuksa::commandRequest* request,
                  kuksa::data* reply) override {
                    auto Processor = handler.getGrpcProcessor();
                    std::string token;
                    grpcHandler::read(request->path_(),token);
                    auto channel = request->channel_();
                    auto result = Processor->processAuthorize(channel,request->reqid_(),token);
                    reply->mutable_channel_()->MergeFrom(channel);
                    reply->set_value_(result);
                    return Status::OK;}

  Status SetValue(ServerContext* context, const kuksa::commandRequest* request,
                  kuksa::data* reply) override {
                    auto Processor = handler.getGrpcProcessor();
                    jsoncons::json req_json;
                    req_json["action"] = "set";
                    req_json["path"] = request->path_();
                    req_json["value"] = request->value_();
                    req_json["requestId"] = request->reqid_();
                    auto channel = request->channel_();
                    auto result = Processor->processSet2(channel,req_json);
                    reply->mutable_channel_()->MergeFrom(channel);
                    reply->set_value_(result);
                    return Status::OK;}

  Status GetValue(ServerContext* context, const kuksa::commandRequest* request,
                  kuksa::data* reply) override {
                    auto Processor = handler.getGrpcProcessor();
                    jsoncons::json req_json;
                    req_json["action"] = "get";
                    req_json["path"] = request->path_();
                    req_json["requestId"] = request->reqid_();
                    auto channel = request->channel_();
                    auto result = Processor->processGet2(channel,req_json);
                    reply->mutable_channel_()->MergeFrom(channel);
                    reply->set_value_(result);
                    return Status::OK;}
};

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
      : stub_(viss_client::NewStub(channel)) {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        std::stringstream ss;
        ss << uuid;
        reqID = ss.str();
        kuksa_channel = addClient(uuid);
        request.set_reqid_(reqID);
  }

  std::string GetMetaData(std::string vssPath){
    ClientContext context;
    request.set_path_(vssPath);
    Status status = stub_->GetMetaData(&context, request, &reply);
    if (status.ok()) {
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string AuthorizeChannel(std::string file){
    ClientContext context;
    request.set_path_(file);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    Status status = stub_->AuthorizeChannel(&context, request, &reply);
    if (status.ok()) {
      kuksa_channel = reply.channel_();
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string getValue(std::string vssPath){
    ClientContext context;
    request.set_path_(vssPath);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    Status status = stub_->GetValue(&context, request, &reply);
    if (status.ok()) {
      kuksa_channel = reply.channel_();
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string setValue(std::string vssPath, std::int32_t value){
    ClientContext context;
    request.set_path_(vssPath);
    request.set_value_(value);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    Status status = stub_->SetValue(&context, request, &reply);
    if (status.ok()) {
      kuksa_channel = reply.channel_();
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }
 private:
  // Container for the data we send to the server.
  kuksa::commandRequest request;
  // Container for the data we expect from the server.
  kuksa::data reply;
  std::unique_ptr<viss_client::Stub> stub_;
  kuksa::kuksaChannel kuksa_channel;
  std::string reqID;
};

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

  std::string command;
  std::string reply;
  std::string abort = "quit";
  // prepare for getting a command while command is not quit
  while(abort.compare(reply) != 0){
    std::cout << "Test-Client>";
    // Get command input from inputfield names
    std::getline(std::cin,command);
    vector<string> temp = split(command);
    // handles which rpc call is made
    if(temp[0] == "getMetaData"){
      reply = connGrpcSes.GetMetaData(temp[1]);
      std::cout << reply << std::endl;
    }
    else if(temp[0] == "authorize"){
      reply = connGrpcSes.AuthorizeChannel(temp[1]);
      std::cout << reply << std::endl;
    }
    else if(temp[0] == "getValue"){
      reply = connGrpcSes.getValue(temp[1]);
      std::cout << reply << std::endl;
    }
    else if(temp[0] == "setValue"){
      reply = connGrpcSes.setValue(temp[1],static_cast<int32_t>(stoi(temp[2])));
      std::cout << reply << std::endl;
    }
    else if(temp[0] == "quit"){
      reply = temp[0];
    }
    else{
      std::cout << "Invalid command!" << std::endl;
    }
  }
}

grpcHandler::grpcHandler() = default;

grpcHandler::~grpcHandler(){
    handler.grpcServer->Shutdown();
}
