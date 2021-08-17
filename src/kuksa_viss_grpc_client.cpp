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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "grpcHandler.hpp"
#include "kuksa.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kuksa::viss_client;

// adds a new channel for gRPC
kuksa::kuksaChannel& addClient(boost::uuids::uuid &uuid){
  auto newChannel = std::make_shared<kuksa::kuksaChannel>();
  uint64_t value = reinterpret_cast<uint64_t>(uuid.data);
  newChannel->set_connectionid(value);
  newChannel->set_typeofconnection(kuksa::kuksaChannel_Type_GRPC);
  return *newChannel;
}

// handles gRPC Connections
// provides setValue, getValue, getMetaData, authorizeChannel as services
// kuksa_channel is for authorizing against the db
class GrpcConnection {
 public:
  GrpcConnection(std::shared_ptr<Channel> channel)
      : stub_(viss_client::NewStub(channel)) {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        std::stringstream ss;
        ss << uuid;
        reqID = ss.str();
        kuksa_channel = addClient(uuid);
  }

  ~GrpcConnection(){
    kuksa_channel.~kuksaChannel();
  }


  std::string GetMetaData(std::string vssPath){
    kuksa::getMetaDataRequest request;
    kuksa::metaData reply;
    ClientContext context;
    request.set_path_(vssPath);
    request.set_reqid_(reqID);
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
    kuksa::authorizeRequest request;
    kuksa::authStatus reply;
    ClientContext context;
    std::string token;
    grpcHandler::read(file,token);
    request.set_token_(token);
    request.set_reqid_(reqID);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    Status status = stub_->AuthorizeChannel(&context, request, &reply);
    if (status.ok()) {
      kuksa_channel = reply.channel_();
      return reply.status_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string getValue(std::string vssPath){
    kuksa::getRequest request;
    kuksa::value reply;
    ClientContext context;
    request.set_path_(vssPath);
    request.set_reqid_(reqID);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    Status status = stub_->GetValue(&context, request, &reply);
    if (status.ok()) {
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string setValue(std::string vssPath, std::string value){
    kuksa::setRequest request;
    kuksa::setStatus reply;
    ClientContext context;
    request.set_path_(vssPath);
    request.set_value_(value);
    request.set_reqid_(reqID);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    Status status = stub_->SetValue(&context, request, &reply);
    if (status.ok()) {
      return reply.status_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }
 private:
  std::unique_ptr<viss_client::Stub> stub_;
  kuksa::kuksaChannel kuksa_channel;
  std::string reqID;
};

// splits the input by a delimiter if no delimiter is provided uses " " as delimiter
std::vector<std::string> split(std::string const& input, std::string const& separator = " ")
{
    std::vector<std::string> result;
    std::string::size_type position, start = 0;

    while (std::string::npos != (position = input.find(separator, start)))
    {
      result.push_back(input.substr(start, position-start));
      start = position + separator.size();
    }
  
    result.push_back(input.substr(start));
    return result;
}

// starts a new client and handles input from command line
void startClient(std::string port, std::string certPath, bool allowInsecureConn = false){
  // insecure client
  if(allowInsecureConn){
    GrpcConnection connGrpcSes(
        grpc::CreateChannel(port, grpc::InsecureChannelCredentials()));
    std::string command;
    std::string reply;
    // prepare for getting a command while command is not quit
    while(reply != "quit"){
      std::cout << "Test-Client>";
      // Get command input from inputfield names
      std::getline(std::cin,command);
      std::vector<std::string> temp = split(command,"\"");
      std::vector<std::string> msg = split(temp[0]);
      // handles which rpc call is made
      if(msg[0] == "getMetaData"){
        if(msg.size() < 2){
          std::cout << "You have to specify a path" << std::endl;
        }
        else{
          reply = connGrpcSes.GetMetaData(msg[1]);
          std::cout << reply << std::endl;
        }
      }
      else if(msg[0] == "authorize"){
        if(msg.size() < 2){
            std::cout << "You have to specify a token" << std::endl;
          }
          else{
            reply = connGrpcSes.AuthorizeChannel(msg[1]);
            std::cout << reply << std::endl;
          }
        }
      else if(msg[0] == "getValue"){
          if(msg.size() < 2){
            std::cout << "You have to specify a path" << std::endl;
          }
          else{
            reply = connGrpcSes.getValue(msg[1]);
            std::cout << reply << std::endl;
          } 
      }
      else if(msg[0] == "setValue"){
          if(msg.size() < 2){
            std::cout << "You have to specify a path" << std::endl;
          }
          if(temp.size() < 2){
            std::cout << "You have to specify a value" << std::endl;
          }
          else{
            reply = connGrpcSes.setValue(msg[1],temp[1]);
            std::cout << reply << std::endl;
          }
      }
      else if(msg[0] == "quit"){
          reply = temp[0];
        }
        else{
          std::cout << "Invalid command!" << std::endl;
      }
    }
  }
  // SSL client
  else{
    std::string cert; 
    std::string key;
    std::string root;
    std::string clientpemfile_ = certPath + "/Client.pem";
    std::string clientkeyfile_ = certPath + "/Client.key";
    std::string clientrootfile_ = certPath + "/CA.pem";
    grpcHandler::read (clientpemfile_, cert);
    grpcHandler::read (clientkeyfile_, key);
    grpcHandler::read (clientrootfile_, root);
    grpc::SslCredentialsOptions opts; 
    opts.pem_cert_chain = cert;
    opts.pem_private_key = key;
    opts.pem_root_certs = root;
    GrpcConnection connGrpcSes(
        grpc::CreateChannel(port, grpc::SslCredentials(opts)));
    std::string command;
    std::string reply;
    // prepare for getting a command while command is not quit
    while(reply != "quit"){
      std::cout << "Test-Client>";
      // Get command input from inputfield names
      std::getline(std::cin,command);
      std::vector<std::string> temp = split(command,"\"");
      std::vector<std::string> msg = split(temp[0]);
      // handles which rpc call is made
      if(msg[0] == "getMetaData"){
        if(msg.size() < 2){
          std::cout << "You have to specify a path" << std::endl;
        }
        else{
          reply = connGrpcSes.GetMetaData(msg[1]);
          std::cout << reply << std::endl;
        }
      }
      else if(msg[0] == "authorize"){
        if(msg.size() < 2){
          std::cout << "You have to specify a token" << std::endl;
        }
        else{
          reply = connGrpcSes.AuthorizeChannel(msg[1]);
          std::cout << reply << std::endl;
        }
      }
      else if(msg[0] == "getValue"){
        if(msg.size() < 2){
          std::cout << "You have to specify a path" << std::endl;
        }
        else{
          reply = connGrpcSes.getValue(msg[1]);
          std::cout << reply << std::endl;
        } 
      }
      else if(msg[0] == "setValue"){
        if(msg.size() < 2){
          std::cout << "You have to specify a path" << std::endl;
        }
        if(temp.size() < 2){
          std::cout << "You have to specify a value" << std::endl;
        }
        else{
          reply = connGrpcSes.setValue(msg[1],temp[1]);
          std::cout << reply << std::endl;
        }
      }
      else if(msg[0] == "quit"){
        reply = temp[0];
      }
      else{
        std::cout << "Invalid command!" << std::endl;
      }
    }
  }
}

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  bool insecureConn = false;
  boost::program_options::options_description desc{"OPTIONS"};
  desc.add_options()
    ("help,h", "Help screen")
    ("config-file,c", boost::program_options::value<boost::filesystem::path>()->default_value(boost::filesystem::path{"config_grpc_client.ini"}),
      "Configuration file for SSL certificates"
      "Sample of configuration file parameters looks like:\n"
      "cert-path = . \n")
    ("cert-path", boost::program_options::value<boost::filesystem::path>()->required()->default_value(boost::filesystem::path(".")),
      "[mandatory] Directory path where 'Client.pem', 'Client.key', and 'CA.pem' are located. ")
    ("log-level","Log Level")
    ("insecure", boost::program_options::bool_switch()->default_value(false), "By default, `kuksa_viss_grpc_client` establishes only SSL (TLS) secured connections. If provided, `kuksa_viss_grpc_client` shall establish un-secured connections")
    ("target",boost::program_options::value<std::string>()->default_value("127.0.0.1:50051"),"Value for a server connection");
  boost::program_options::variables_map var;
  boost::program_options::store(parse_command_line(argc, argv, desc), var);
  if (var.count("help")) {
    std::cout << "\tCommands available: authorize <path to token>, getMetaData <vss path>, getValue <vss path>, setValue <vss path> <value>" << std::endl;
    std::cout << "\t--config-file default: config_grpc_client.ini | specifies where to search for SSL certificates" << std::endl;
    std::cout << "\t--target default: 127.0.0.1:50051 | specifies where to look for a connection to the gRPC server" << std::endl;
    std::cout << "\t--insecure default: false | if you call the client with --insecure it establishes a insecure connection" << std::endl;
  }
  else{
    if (var.count("config-file")) {
    auto configFile = var["config-file"].as<boost::filesystem::path>();
    auto configFilePath = boost::filesystem::path(configFile);
    std::cout << "Read configs from " <<  configFile.string() << std::endl;
    std::ifstream ifs(configFile.string());
    if (ifs) {
      // update path only, if these options is not defined via command line, but
      // via config file
      boost::program_options::store(parse_config_file(ifs, desc), var);
      auto cert_path = var["cert-path"].as<boost::filesystem::path>();
      var.at("cert-path").value() =
          boost::filesystem::absolute(cert_path, configFilePath.parent_path());
      std::cout << "Update cert-path to "
                << var["cert-path"].as<boost::filesystem::path>().string()
                << std::endl;
    } else{
      std::cerr << "Could not open config file: " << configFile << std::endl;
    }
    // store again, because command line argument prefered
    boost::program_options::store(parse_command_line(argc, argv, desc), var);
    }
    if(var.count("insecure")){
      insecureConn = true;
    }
    startClient(var["target"].as<std::string>(),var["cert-path"].as<boost::filesystem::path>().string(),insecureConn);
  }
  return 0;
}
