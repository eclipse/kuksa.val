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
#include <boost/functional/hash.hpp>
#include <thread>

#include "grpcHandler.hpp"
#include "KuksaChannel.hpp"

#include "kuksa.grpc.pb.h"
#include "ILogger.hpp"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kuksa::kuksa_grpc_if;

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

// Map RequestType -> std::string
const static std::map<kuksa::RequestType, std::string> AttributeStringMap =
{
  {kuksa::RequestType::CURRENT_VALUE, "value"},
  {kuksa::RequestType::TARGET_VALUE, "targetValue"},
};

using grpcSession_t = struct grpcSession{
  std::string peer;
  boost::uuids::uuid sessionId;

    // constructor
  grpcSession(std::string p, boost::uuids::uuid id) {
      this->peer = p;
      this->sessionId = id;
  }

  // Equal operator
  bool operator==(const grpcSession &p) const {
      return this->peer == p.peer && this->sessionId == p.sessionId;
  }
};

struct GrpcSessionHasher
{
  std::size_t operator()(const grpcSession_t& k) const
  {
    boost::hash<boost::uuids::uuid> uuid_hasher;
     return (uuid_hasher(k.sessionId) ^ std::hash<std::string>()(k.peer));
  }
};

// Logic and data behind the servers behaviour
// implementation of the rpc interfaces server side
class RequestServiceImpl final : public kuksa_grpc_if::Service {    

  private:
    std::shared_ptr<ILogger> logger;
    std::unordered_map<grpcSession_t, KuksaChannel, GrpcSessionHasher> grpcSessionMap;

    /* Internal helper function to authorize the session
     *  Will create and assign a KuksaChannel object to the session.
     *  Subsequent calls must retrieve this object to execute a RPC. 
     */
    jsoncons::json authorizeHelper (const std::string peer, const std::string token) {
      // Create an auth request
      jsoncons::json req_json;
      auto uuid = boost::uuids::random_generator()();

      req_json["action"] = "authorize";
      req_json["requestId"] = boost::uuids::to_string(uuid);
      req_json["token"] = token;
      
      auto newChannel = std::make_shared<KuksaChannel>();
      newChannel->setConnID(0);   // Connection ID is used only for websocket connections.
      newChannel->setType(KuksaChannel::Type::GRPC);

      auto Processor = handler.getGrpcProcessor();
      auto response = Processor->processAuthorize(*newChannel, boost::uuids::to_string(uuid), token);
      auto resJson = json::parse(response);

      if (!resJson.contains("error")) { // Success case
        grpcSession_t session = grpcSession(peer, uuid);
        grpcSessionMap[session] = *newChannel;
      }
      return resJson;
    } 

    /* Internal helper function to retrieve KuksaChannel object to the session.
     * Needs the UUID assigned to the session in the context metadata. 
     */
    KuksaChannel* authChecker (ServerContext* context) {
      KuksaChannel* kc = NULL;
      try {
        auto metadata = context->client_metadata();

        boost::uuids::string_generator gen;
        auto iter = metadata.find("connectionid");
        if (iter != metadata.end()) {
            std::string connIdString(iter->second.begin(), iter->second.end());
            auto connId = gen(connIdString);
            grpcSession session = grpcSession(context->peer(), connId);
            kc = &grpcSessionMap[session];
        }
      } catch (std::exception &e) {
        // Do Nothing
        // Most probably KuksaChannel does not exist for this session.
        // This will result in authorization error later.
        logger->Log(LogLevel::ERROR,e.what());
      }
      return kc;
    }

    /* Internal function to handle subscribe read streams
     * 
     */
    static void subscribeReadStreamHandling(::grpc::ServerReaderWriter<::kuksa::SubscribeResponse, ::kuksa::SubscribeRequest>* stream) {
      
    }

    /* Internal function to handle subscribe write streams
     * 
     */
    static void subscribeWriteStreamHandling(::grpc::ServerReaderWriter<::kuksa::SubscribeResponse, ::kuksa::SubscribeRequest>* stream) {
      
    }
  public: 

    RequestServiceImpl(std::shared_ptr<ILogger> _logger) {
      logger = _logger;
    }

    Status get(ServerContext* context, const kuksa::GetRequest* request, kuksa::GetResponse* reply) override { 
      jsoncons::json req_json;
      stringstream msg;
      msg << "gRPC get invoked with type " << kuksa::RequestType_Name(request->type()) << " by " << context->peer(); 
      logger->Log(LogLevel::INFO,msg.str());

      // Check if authorized and get the corresponding KuksaChannel
      KuksaChannel* kc = authChecker(context); 
      if ((kc == NULL) && (request->type() != kuksa::RequestType::METADATA)){
        reply->mutable_status()->set_statuscode(404);
        reply->mutable_status()->set_statusdescription("No Authorization.");
        return Status::OK;
      }

      // Return if no paths are given
      if (request->path().size() == 0) {
        reply->mutable_status()->set_statuscode(400);
        reply->mutable_status()->set_statusdescription("No valid path found.");
        return Status::OK;
      }

      req_json["action"] = "get";
      if (request->type() == kuksa::RequestType::METADATA) {
        req_json["action"] = "getMetaData";
      }

      auto iter = AttributeStringMap.find(request->type());
      std::string attr;
      if (iter != AttributeStringMap.end()) {
        attr = iter->second;
        req_json["attribute"] = attr;
      } else {
        attr = "value";
      }

      bool singleFailure = false;

      for (int i=0; i < request->path().size(); i++) {
        req_json["requestId"] = boost::uuids::to_string(boost::uuids::random_generator()());
        req_json["path"] = request->path()[i];

        try {
          auto Processor = handler.getGrpcProcessor();
          std::string result;
          if (request->type() != kuksa::RequestType::METADATA) { 
            result = Processor->processGet2(*kc, req_json);
          } else {
            result = Processor->processGetMetaData(req_json);
          }
          auto resJson = json::parse(result);
          if (resJson.contains("error")) { // Failure Case
            uint32_t code = resJson["error"]["number"].as<unsigned int>(); 
            std::string reason = resJson["error"]["reason"].as_string() + " " + resJson["error"]["message"].as_string();
            reply->mutable_status()->set_statuscode(code);
            reply->mutable_status()->set_statusdescription(reason);
            singleFailure = true;
          } else { // Success Case
            auto val = reply->add_values();
            if (request->type() != kuksa::RequestType::METADATA) {
              val->set_valuestring(resJson["data"]["dp"][attr].as_string());
              val->set_timestamp(resJson["data"]["dp"]["ts"].as_string());
            } else {
              val->set_valuestring(resJson["metadata"].as_string());
            }
            val->set_path(resJson["data"]["path"].as_string());
          }
        } catch (std::exception &e) {
          singleFailure = true;
        }
      }

      if (singleFailure && request->path().size() > 1) {
        reply->mutable_status()->set_statuscode(400);
        reply->mutable_status()->set_statusdescription("One or more paths could not be resolved. Try individual requests.");
      } else if (!singleFailure) {
        reply->mutable_status()->set_statuscode(200);
        reply->mutable_status()->set_statusdescription("Get request successfully processed");
      }
      return Status::OK;
    }

    Status set(ServerContext* context, const kuksa::SetRequest* request, kuksa::SetResponse* reply) override {                                       
      jsoncons::json req_json;
      stringstream msg;
      msg << "gRPC set invoked with type " << kuksa::RequestType_Name(request->type()) << " by " << context->peer(); 
      logger->Log(LogLevel::INFO,msg.str());

      // Check if authorized and get the corresponding KuksaChannel
      KuksaChannel* kc = authChecker(context); 
      if (kc == NULL) {
        reply->mutable_status()->set_statuscode(404);
        reply->mutable_status()->set_statusdescription("No Authorization!.");
        return Status::OK;
      }

      if (request->type() == kuksa::RequestType::METADATA) {
        // Do Nothing!!
        // Setting Metadata is not supported
      } else {
        req_json["action"] = "set";
        auto iter = AttributeStringMap.find(request->type());
        std::string attr;
        if (iter != AttributeStringMap.end()) {
          attr = iter->second;
        } else {
          attr = "value";
        }
        req_json["attribute"] = attr;
        bool singleFailure = false;

        for (int i=0; i < request->values().size(); i++) {
          auto val = request->values()[i];
          req_json["requestId"] = boost::uuids::to_string(boost::uuids::random_generator()());
          req_json["path"] = val.path();
          req_json[attr] = val.valuestring();

          try {
            auto Processor = handler.getGrpcProcessor();
            auto result = Processor->processSet2(*kc, req_json);
            auto resJson = json::parse(result);
            if (resJson.contains("error")) { // Failure Case
              uint32_t code = resJson["error"]["number"].as<unsigned int>();
              std::string reason = resJson["error"]["reason"].as_string() + " " + resJson["error"]["message"].as_string();
              reply->mutable_status()->set_statuscode(code);
              reply->mutable_status()->set_statusdescription(reason);
              singleFailure = true;
            } else { // Success Case
              reply->mutable_status()->set_statuscode(200);
              reply->mutable_status()->set_statusdescription("Set request successfully processed");
            }
          } catch (std::exception &e) {
            singleFailure = true;
          }
        }

        if (singleFailure && request->values().size() > 1) {
          reply->mutable_status()->set_statuscode(400);
          reply->mutable_status()->set_statusdescription("One or more paths could not be resolved. Try individual requests.");
        } else if (!singleFailure) {
          reply->mutable_status()->set_statuscode(200);
          reply->mutable_status()->set_statusdescription("Set request successfully processed.");
        }
      }
      return Status::OK;
    }

    Status subscribe(ServerContext* context, ::grpc::ServerReaderWriter<::kuksa::SubscribeResponse, ::kuksa::SubscribeRequest>* stream) {
      stringstream msg;
      msg << "gRPC subscribe invoked" << " by " << context->peer(); 
      logger->Log(LogLevel::INFO,msg.str());

      // Check if authorized and get the corresponding KuksaChannel
      KuksaChannel* kc = authChecker(context); 
      if (kc == NULL) {
        return Status::OK;
      }

      thread readStreamThread (subscribeReadStreamHandling, stream);
      thread writeStreamThread (subscribeWriteStreamHandling, stream);
      writeStreamThread.join();
      readStreamThread.join();
      return Status::OK;
    }

    Status authorize(ServerContext* context, const kuksa::AuthRequest* request, kuksa::AuthResponse* reply) override {                                       
      stringstream msg;
      msg << "gRPC authorize invoked with token " << request->token(); 
      logger->Log(LogLevel::INFO,msg.str());
      logger->Log(LogLevel::INFO,context->peer());

      auto resJson = authorizeHelper(context->peer(), request->token());
      
      // Populate the response
      if (!resJson.contains("error")) { // Success case
        reply->mutable_status()->set_statuscode(200);
        reply->mutable_status()->set_statusdescription("Authorization Successful.");
        reply->set_connectionid(resJson["requestId"].as_string());
      } else { // Failure case
        uint32_t code = resJson["error"]["number"].as<unsigned int>();
        std::string reason = resJson["error"]["reason"].as_string() + " " + resJson["error"]["message"].as_string();
        reply->mutable_status()->set_statuscode(code);
        reply->mutable_status()->set_statusdescription(reason);
      }     
      return Status::OK;
    }  
};

void grpcHandler::RunServer(std::shared_ptr<VssCommandProcessor> Processor, std::shared_ptr<ILogger> logger_, std::string certPath, bool allowInsecureConn) {
  string server_address("0.0.0.0:50051");
  RequestServiceImpl service(logger_);

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