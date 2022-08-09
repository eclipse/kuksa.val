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

#ifndef __MQTTPUBLISHER_H__
#define __MQTTPUBLISHER_H__

#include "IPublisher.hpp"
#include <regex>
#include "mosquitto.h"
#include <boost/program_options.hpp>

class ILogger;

/**
 * \class MQTTPublisher 
 * \brief A MQTT client as publisher
 *        Implementation of \ref IClient interface, based on mosquittopp library
 */
class MQTTPublisher : public IPublisher
 {
  private:
    std::shared_ptr<ILogger> logger_;
    std::vector<std::string> paths_;
    int keepalive_;
    int qos_;
    int connection_retry_;
    struct mosquitto *mosq_ = nullptr;
    bool isConnected_ = false;
    std::string prefix_{};
    const std::string host_;
    const int port_;

    /**
     * @brief initialize mosquitto mqtt client
     */
    void init(const std::string&, bool insecure);
    /**
     * @brief Set username and password for this mqtt client
     */
    bool setUsernamePassword(const 	std::string& username, const std::string& password);

    void addPrefix(const std::string& prefix);
    bool start();

  public:
    /**
     * @brief Initialize Boost.Beast server
     * @param loggerUtil Hostname for server connection
     * @param id client id
     * @param host broker host
     * @param port Port where to wait for connections
     * @param keepalive the number of seconds after which the broker should send a PING
     * @param qos integer value 0, 1 or 2 indicating the Quality of Service to be used for the message
     *              message to the client if no other messages have been exchanged
     *              in that time. Default is 60s
     */
    MQTTPublisher(std::shared_ptr<ILogger> loggerUtil, const std::string &id, boost::program_options::variables_map& config);
    ~MQTTPublisher();

    static boost::program_options::options_description& getOptions();

    /**
     * @brief Start client
     */
    void addPublishPath(const std::string& path);


    // IPublisher
    bool sendPathValue(const std::string &path, const jsoncons::json &value) override;

};



#endif /* __MQTTPUBLISHER_H__ */

