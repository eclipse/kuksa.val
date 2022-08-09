/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 **********************************************************************/

#ifndef __GRPC_HANDLER_H__
#define __GRPC_HANDLER_H__

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "jsoncons/json.hpp"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "VssCommandProcessor.hpp"
#include "kuksa.grpc.pb.h"
#include "SubscriptionHandler.hpp"


class grpcHandler{
    public:
      static void grpc_send_object_to_stream(std::shared_ptr<ILogger> logger, const std::string& vssdatatype, const jsoncons::json& data, grpc::ServerReaderWriter<kuksa::SubscribeResponse, kuksa::SubscribeRequest>* stream );
      static void grpc_fill_value(std::shared_ptr<ILogger> logger, const std::string& vssdatatype, const jsoncons::json& data, kuksa::Value* grpcvalue, const std::string& attr = "value");
    private:
        std::shared_ptr<grpc::Server> grpcServer;
        std::shared_ptr<VssCommandProcessor> grpcProcessor;
        std::shared_ptr<IVssDatabase> grpcDatabase;
        std::shared_ptr<ILogger> logger_;
       public:
        grpcHandler();
        virtual ~grpcHandler();
        static void RunServer(std::shared_ptr<VssCommandProcessor> Processor, std::shared_ptr<IVssDatabase> database, std::shared_ptr<ISubscriptionHandler> _subhandler, std::shared_ptr<ILogger> logger_, std::string certPath, bool allowInsecureConn);   
        static void read (const std::string& filename, std::string& data); 
        std::shared_ptr<ILogger> getLogger() {
          return this->logger_;
        }
        std::shared_ptr<VssCommandProcessor> getGrpcProcessor() {
          return this->grpcProcessor;
        }
        std::shared_ptr<grpc::Server> getGrpcServer() {
          return this->grpcServer;
        }
};

#endif