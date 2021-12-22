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
    std::cout << "Failed to read SSL certificate" + string(filename) << std::endl;
  }
	return;
}

// Logic and data behind the servers behaviour
// implementation of the rpc interfaces server side
class RequestServiceImpl final : public viss_client::Service {                                          
  Status GetMetaData(ServerContext* context, const kuksa::getMetaDataRequest* request,
                  kuksa::metaData* reply) override {
                    jsoncons::json req_json;
                    req_json["action"] = "getMetaData";
                    req_json["path"] = request->path_();
                    req_json["requestId"] = request->reqid_();
                    auto Processor = handler.getGrpcProcessor();
                    auto result = Processor->processGetMetaData(req_json);
                    reply->set_value_(result);
                    return Status::OK;}

  Status AuthorizeChannel(ServerContext* context, const kuksa::authorizeRequest* request,
                  kuksa::authStatus* reply) override {
                    auto Processor = handler.getGrpcProcessor();
                    auto channel = request->channel_();
                    auto result = Processor->processAuthorize(channel,request->reqid_(),request->token_());
                    reply->mutable_channel_()->MergeFrom(channel);
                    reply->set_status_(result);
                    return Status::OK;}

  Status SetValue(ServerContext* context, const kuksa::setRequest* request,
                  kuksa::setStatus* reply) override {
                    auto Processor = handler.getGrpcProcessor();
                    jsoncons::json req_json;
                    req_json["action"] = "set";
                    req_json["path"] = request->path_();
                    req_json["value"] = request->value_();
                    req_json["requestId"] = request->reqid_();
                    auto channel = request->channel_();
                    auto result = Processor->processSet2(channel,req_json);
                    reply->set_status_(result);
                    return Status::OK;}

  Status GetValue(ServerContext* context, const kuksa::getRequest* request,
                  kuksa::value* reply) override {
                    auto Processor = handler.getGrpcProcessor();
                    jsoncons::json req_json;
                    req_json["action"] = "get";
                    req_json["path"] = request->path_();
                    req_json["requestId"] = request->reqid_();
                    auto channel = request->channel_();
                    auto result = Processor->processGet2(channel,req_json);
                    reply->set_value_(result);
                    return Status::OK;}
};

void grpcHandler::RunServer(std::shared_ptr<VssCommandProcessor> Processor, std::shared_ptr<ILogger> logger_, std::string certPath, bool allowInsecureConn) {
  string server_address("127.0.0.1:50051");
  RequestServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;

  if(allowInsecureConn){
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  }
  else{
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
  }

  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server
  handler.grpcServer = builder.BuildAndStart();
  handler.grpcProcessor = Processor;
  handler.logger_=logger_;
  handler.logger_->Log(LogLevel::INFO, "Kuksa viss gRPC server Version 1.0.0");
  handler.logger_->Log(LogLevel::INFO, "gRPC Server listening on " + string(server_address));

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  handler.getGrpcServer()->Wait();
}

grpcHandler::grpcHandler() = default;

grpcHandler::~grpcHandler() = default;
