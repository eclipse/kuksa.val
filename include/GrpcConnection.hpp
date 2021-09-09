#include "kuksa.grpc.pb.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#ifndef gRPCConnection_
#define gRPCConnection_

class GrpcConnection {
 public:
  GrpcConnection(std::shared_ptr<grpc::Channel> channel);
  ~GrpcConnection();


  std::string GetMetaData(std::string vssPath);

  std::string AuthorizeChannel(std::string file);

  std::string getValue(std::string vssPath);

  std::string setValue(std::string vssPath, std::string value);

  std::string getReqID() {
    return this->reqID_;
  }

  void setStubtest_(kuksa::viss_client::StubInterface* stub) { this->stubtest_ = stub;}
  void setCMAKE_TESTING_ENABLED(bool CMAKE_TESTING_ENABLED) {
    this->CMAKE_TESTING_ENABLED_ = CMAKE_TESTING_ENABLED;
  }
  void setReqID(std::string reqID){ this->reqID_ = reqID;}
 private:
  boost::uuids::uuid uuid;
  std::unique_ptr<kuksa::viss_client::Stub> stub_;
  kuksa::viss_client::StubInterface* stubtest_;
  kuksa::kuksaChannel kuksa_channel;
  std::string reqID_;
  bool CMAKE_TESTING_ENABLED_ = false;
};

#endif