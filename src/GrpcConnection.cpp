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
#include "GrpcConnection.hpp"

class grpcConnectionHandler {
    private:
        std::mutex mconnGrpc;
        std::unordered_map<uint64_t, std::shared_ptr<kuksa::kuksaChannel>> connGrpc;
    public:
        kuksa::kuksaChannel& addClient(boost::uuids::uuid uuid){
          auto newChannel = std::make_shared<kuksa::kuksaChannel>();
          std::lock_guard<std::mutex> lock(mconnGrpc);
          uint64_t value = reinterpret_cast<uint64_t>(uuid.data);
          newChannel->set_connectionid(value);
          newChannel->set_typeofconnection(kuksa::kuksaChannel_Type_GRPC);
          connGrpc[reinterpret_cast<uint64_t>(uuid.data)] = newChannel;
          return *newChannel;
        }
        void RemoveClient(const boost::uuids::uuid uuid){
          std::lock_guard<std::mutex> lock(mconnGrpc);
          connGrpc.erase(reinterpret_cast<uint64_t>(uuid.data));
        }
        std::unordered_map<uint64_t, std::shared_ptr<kuksa::kuksaChannel>> getConnGrpc() {
            return this->connGrpc;
        }
        grpcConnectionHandler() = default;
        ~grpcConnectionHandler() = default;
};

grpcConnectionHandler connGrpcHandler;

// handles gRPC Connections
// provides setValue, getValue, getMetaData, authorizeChannel as services
// kuksa_channel is for authorizing against the db
GrpcConnection::GrpcConnection(std::shared_ptr<grpc::Channel> channel){
        stub_ = kuksa::viss_client::NewStub(channel);
        uuid = boost::uuids::random_generator()();
        std::stringstream ss;
        ss << uuid;
        reqID_ = ss.str();
        kuksa_channel = connGrpcHandler.addClient(uuid);
}

GrpcConnection::~GrpcConnection(){
    kuksa_channel.~kuksaChannel();
    connGrpcHandler.RemoveClient(uuid);
}

std::string GrpcConnection::GetMetaData(std::string vssPath){
    kuksa::getMetaDataRequest request;
    kuksa::metaData reply;
    grpc::ClientContext context;
    request.set_path_(vssPath);
    request.set_reqid_(reqID_);
    // for checking if a Unit Test calls this function. Then the server gets mocked by gmock
    // for detailed documentation see https://grpc.github.io/grpc/cpp/md_doc_unit_testing.html
    grpc::Status status;
    if(CMAKE_TESTING_ENABLED_){
      status = stubtest_->GetMetaData(&context, request, &reply);
    }
    else{
      status = stub_->GetMetaData(&context, request, &reply);
    }
    if (status.ok()) {
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
}

std::string GrpcConnection::AuthorizeChannel(std::string file){
    kuksa::authorizeRequest request;
    kuksa::authStatus reply;
    grpc::ClientContext context;
    std::string token;
    grpcHandler::read(file,token);
    request.set_token_(token);
    request.set_reqid_(reqID_);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    grpc::Status status;
    if(CMAKE_TESTING_ENABLED_){
      status = stubtest_->AuthorizeChannel(&context, request, &reply);
    }
    else{
      status = stub_->AuthorizeChannel(&context, request, &reply);
    }
    if (status.ok()) {
      kuksa_channel = reply.channel_();
      return reply.status_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
}

std::string GrpcConnection::getValue(std::string vssPath){
    kuksa::getRequest request;
    kuksa::value reply;
    grpc::ClientContext context;
    request.set_path_(vssPath);
    request.set_reqid_(reqID_);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
    grpc::Status status;
    if(CMAKE_TESTING_ENABLED_){
      status = stubtest_->GetValue(&context, request, &reply);
    }
    else{
      status = stub_->GetValue(&context, request, &reply);
    }
    if (status.ok()) {
      return reply.value_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
}

std::string GrpcConnection::setValue(std::string vssPath, std::string value){
    kuksa::setRequest request;
    kuksa::setStatus reply;
    grpc::ClientContext context;
    request.set_path_(vssPath);
    request.set_value_(value);
    request.set_reqid_(reqID_);
    request.mutable_channel_()->MergeFrom(kuksa_channel);
     grpc::Status status;
    if(CMAKE_TESTING_ENABLED_){
      status = stubtest_->SetValue(&context, request, &reply);
    }
    else{
      status = stub_->SetValue(&context, request, &reply);
    }
    if (status.ok()) {
      return reply.status_();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
}