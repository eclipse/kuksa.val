/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "VssCommandProcessor.hpp"

class grpcHandler{
    private:
        std::shared_ptr<grpc::Server> grpcServer;
        std::shared_ptr<VssCommandProcessor> grpcProcessor;

    public:
        grpcHandler();
        virtual ~grpcHandler();
        static void RunServer(std::shared_ptr<VssCommandProcessor> Processor, std::string certPath);   
        static void read (const std::string& filename, std::string& data); 
        static void startClient(std::string port, std::string certPath);
        std::shared_ptr<VssCommandProcessor> getGrpcProcessor() {
          return this->grpcProcessor;
        }
};