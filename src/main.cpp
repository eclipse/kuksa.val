/*
 * ******************************************************************************
 * Copyright (c) 2019-2020 Robert Bosch GmbH.
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

#include <gio/gio.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <exception>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <jsoncons/json.hpp>
#include <jsonpath/json_query.hpp>

#include "AccessChecker.hpp"
#include "Authenticator.hpp"
#include "BasicLogger.hpp"
#include "RestV1ApiHandler.hpp"
#include "SubscriptionHandler.hpp"
#include "VssCommandProcessor.hpp"
#include "VssDatabase.hpp"
#include "WebSockHttpFlexServer.hpp"
#include "MQTTClient.hpp"
#include "exception.hpp"

#include "../buildinfo.h"

using namespace std;
using namespace boost;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;

// Websocket port
#define PORT 8090

static VssDatabase *gDatabase = NULL;

static GDBusNodeInfo *introspection_data = NULL;

/* Introspection data for the service we are exporting */
static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.eclipse.kuksa.w3cbackend'>"
    "    <method name='pushUnsignedInt'>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='t' name='value' direction='in'/>"
    "    </method>"
    "    <method name='pushInt'>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='x' name='value' direction='in'/>"
    "    </method>"
    "    <method name='pushDouble'>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='d' name='value' direction='in'/>"
    "    </method>"
    "    <method name='pushBool'>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='b' name='value' direction='in'/>"
    "    </method>"
    "    <method name='pushString'>"
    "      <arg type='s' name='path' direction='in'/>"
    "      <arg type='s' name='value' direction='in'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static void handle_method_call(GDBusConnection *connection, const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name, GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {
  (void)connection;
  (void)object_path;
  (void)interface_name;
  (void)parameters;
  (void)user_data;
  (void)sender;

  json jsonVal;
  const gchar *vss_path = NULL;

  if (gDatabase == NULL) {
    g_dbus_method_invocation_return_error(invocation, G_IO_ERROR,
                                          G_IO_ERROR_FAILED_HANDLED,
                                          "No database instance yet.");
    return;
  }

  if (g_strcmp0(method_name, "pushUnsignedInt") == 0) {
    guint64 value;
    g_variant_get(parameters, "(&st)", &vss_path, &value);
    jsonVal = value;
    g_message("pushUnsignedInt called with param ( %s , %ld )", vss_path,
              value);
  } else if (g_strcmp0(method_name, "pushInt") == 0) {
    gint64 value;
    g_variant_get(parameters, "(&sx)", &vss_path, &value);
    jsonVal = value;
    g_message("PushInt called with param ( %s , %ld )", vss_path, value);
  } else if (g_strcmp0(method_name, "pushDouble") == 0) {
    gdouble value;
    g_variant_get(parameters, "(&sd)", &vss_path, &value);
    jsonVal = value;
    g_message("PushDouble called with param ( %s , %f )", vss_path, value);
  }

  else if (g_strcmp0(method_name, "pushBool") == 0) {
    gboolean value;
    g_variant_get(parameters, "(&sb)", &vss_path, &value);
    jsonVal = value;
    g_message("pushBool called with param ( %s , %d )", vss_path, value);

  }

  else if (g_strcmp0(method_name, "pushString") == 0) {
    const gchar *value;
    g_variant_get(parameters, "(&ss)", &vss_path, &value);
    jsonVal = std::string(value);
    g_message("pushString called with param ( %s , %s )", vss_path, value);

  } else {
    g_dbus_method_invocation_return_error(
        invocation, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED,
        "Handling needs to be implemented here.");
  }

  // set the data in the db.
  try {
    string pathStr(vss_path);
    gDatabase->setSignal(pathStr, jsonVal);
  } catch (genException &e) {
    g_dbus_method_invocation_return_error(
        invocation, G_IO_ERROR, G_IO_ERROR_FAILED_HANDLED,
        "Exception occured while setting data to the server.");
  }

  g_dbus_method_invocation_return_value(invocation, NULL);
}

/* for now */
static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    NULL,
    NULL,
    /* Padding for future expansion */
    {NULL}};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
                            gpointer user_data) {
  (void)name;
  (void)user_data;
  guint registration_id;
  registration_id = g_dbus_connection_register_object(
      connection, "/org/eclipse/kuksaW3Cbackend",
      introspection_data->interfaces[0], &interface_vtable, NULL,
      NULL,  /* user_data_free_func */
      NULL); /* GError** */
  g_assert(registration_id > 0);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name,
                             gpointer user_data) {
  (void)user_data;
  (void)connection;
  printf("Bus Name acquired %s", name);
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
                         gpointer user_data) {
  (void)user_data;
  (void)connection;
  (void)name;
  exit(1);
}

static void print_usage(const char *prog_name,
                        program_options::options_description &desc) {
  cerr << "Usage: " << prog_name << " OPTIONS" << endl;
  cerr << desc << std::endl;
}

int main(int argc, const char *argv[]) {
  vector<string> logLevels{"NONE"};
  uint8_t logLevelsActive = static_cast<uint8_t>(LogLevel::NONE);

  std::cout << "kuksa.val server" << std::endl;
  std::cout << "Commit "  << string(GIT_HEAD_SHA1).substr(0,7);
  if (GIT_IS_DIRTY)
    std::cout << "-dirty";
  std::cout << " from " << GIT_COMMIT_DATE_ISO8601 << std::endl;

  program_options::options_description desc{"Options"};
  desc.add_options()
    ("help,h", "Help screen")
    ("config-file,cfg", program_options::value<boost::filesystem::path>()->default_value(boost::filesystem::path{"config.ini"}),
      "Configuration file with W3C-Server input parameters."
      "Configuration file can replace command-line parameters and through different files multiple configurations can be handled more easily (e.g. test and production setup)."
      "Sample of configuration file parameters looks like:\n"
      "vss=vss_rel_2.0.json\n"
      "cert-path=. \n"
      "insecure=true \n"
      "log-level=ALL\n")
    ("vss", program_options::value<boost::filesystem::path>()->required(), "[mandatory] Path to VSS data file describing VSS data tree structure which W3C-Server shall handle. Sample 'vss_rel_2.0.json' file can be found under [unit-test](./unit-test/vss_rel_2.0.json)")
    ("cert-path", program_options::value<boost::filesystem::path>()->required()->default_value(boost::filesystem::path(".")),
      "[mandatory] Directory path where 'Server.pem', 'Server.key' and 'jwt.key.pub' are located. ")
    ("insecure", program_options::bool_switch()->default_value(false), "By default, W3C-Server shall accept only SSL (TLS) secured connections. If provided, W3C-Server shall also accept plain un-secured connections for Web-Socket and REST API connections, and also shall not fail connections due to self-signed certificates.")
    ("use-keycloak", "Use KeyCloak for permission management")
    ("address", program_options::value<string>()->default_value("127.0.0.1"),
      "If provided, W3C-Server shall use different server address than default _'localhost'_")
    ("port", program_options::value<int>()->default_value(8090),
        "If provided, W3C-Server shall use different server port than default '8090' value")
    ("log-level",
      program_options::value<vector<string>>(&logLevels)->composing(),
      "Enable selected log level value. To allow for different log level combinations, parameter can be provided multiple times with different log level values.\n"
      "Supported log levels: NONE, VERBOSE, INFO, WARNING, ERROR, ALL");
  // mqtt options 
  program_options::options_description mqtt_desc("MQTT Options");
  mqtt_desc.add_options()
    ("mqtt.insecure", program_options::bool_switch()->default_value(false), "Do not check that the server certificate hostname matches the remote hostname. Do not use this option in a production environment")
    ("mqtt.username", program_options::value<string>(), "Provide a mqtt username")
    ("mqtt.password", program_options::value<string>(), "Provide a mqtt password")
    ("mqtt.address", program_options::value<string>()->default_value("localhost"),
    "Address of MQTT broker")
    ("mqtt.port", program_options::value<int>()->default_value(1883),
    "Port of MQTT broker")
    ("mqtt.qos", program_options::value<int>()->default_value(0), "Quality of service level to use for all messages. Defaults to 0")
    ("mqtt.keepalive", program_options::value<int>()->default_value(60), "Keep alive in seconds for this mqtt client. Defaults to 60")
    ("mqtt.retry", program_options::value<int>()->default_value(3), "Times of retry via connections. Defaults to 3")
    ("mqtt.topic-prefix", program_options::value<string>(), "Prefix to add for each mqtt topics")
    ("mqtt.publish", program_options::value<string>()->default_value(""), "List of vss data path (using readable format with `.`) to be published to mqtt broker, using \";\" to seperate multiple path and \"*\" as wildcard");
  desc.add(mqtt_desc);
  program_options::variables_map variables;
  program_options::store(parse_command_line(argc, argv, desc), variables);
  // if config file passed, get configuration from it
  if (variables.count("config-file")) {
    auto configFile = variables["config-file"].as<boost::filesystem::path>();
    auto configFilePath = boost::filesystem::path(configFile);
    std::ifstream ifs(configFile.string());
    if (ifs) {
      // update path only, if these options is not defined via command line, but via config file
      bool update_vss_path = !variables.count("vss");
      bool update_cert_path = (!variables.count("cert-path")) || (!variables["cert-file"].defaulted());
      program_options::store(parse_config_file(ifs, desc), variables);
      if(update_vss_path){
        auto vss_path = variables["vss"].as<boost::filesystem::path>();
        variables.at("vss").value() = boost::filesystem::absolute(vss_path, configFilePath.parent_path());
        std::cout << "Update vss path to " <<  variables["vss"].as<boost::filesystem::path>().string() <<std::endl;
      }
      if(update_cert_path){
        auto cert_path = variables["cert-path"].as<boost::filesystem::path>();
        variables.at("cert-path").value() = boost::filesystem::absolute(cert_path, configFilePath.parent_path());
        std::cout << "Update cert-path to " <<  variables["cert-path"].as<boost::filesystem::path>().string() <<std::endl;
      }
    } else if (!variables["config-file"].defaulted()){
      std::cerr << "Could not open config file: " << configFile << std::endl;
      return -1;
    }
  }

  // print usage
  if (variables.count("help")) {
    print_usage(argv[0], desc);
    return 0;
  }
  program_options::notify(variables);

  for (auto const &token : logLevels) {
    if (token == "NONE")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::NONE);
    else if (token == "VERBOSE")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::VERBOSE);
    else if (token == "INFO")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::INFO);
    else if (token == "WARNING")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::WARNING);
    else if (token == "ERROR")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::ERROR);
    else if (token == "ALL")
      logLevelsActive |= static_cast<uint8_t>(LogLevel::ALL);
    else {
      cerr << "Invalid input parameter for LogLevel" << std::endl;
      return -1;
    }
  }

  // Initialize logging
  auto logger = std::make_shared<BasicLogger>(logLevelsActive);

  try {
    // initialize server

    auto port = variables["port"].as<int>();
    auto insecure = variables[("insecure")].as<bool>();
    auto vss_path = variables["vss"].as<boost::filesystem::path>();

    if (variables.count("use-keycloak")) {
      // Start D-Bus backend connection.
      guint owner_id;
      GMainLoop *loop;

      introspection_data =
          g_dbus_node_info_new_for_xml(introspection_xml, NULL);
      g_assert(introspection_data != NULL);

      owner_id =
          g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.eclipse.kuksa.w3cbackend",
                         G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired,
                         on_name_acquired, on_name_lost, NULL, NULL);

      loop = g_main_loop_new(NULL, FALSE);
      g_main_loop_run(loop);
      g_bus_unown_name(owner_id);
      g_dbus_node_info_unref(introspection_data);
    }

    // initialize pseudo random number generator
    std::srand(std::time(nullptr));

    // define doc root path for server..
    // for now also add 'v1' to designate version 1 of REST API as default
    // in future, we can add/update REST API with new versions but also support
    // older by having API versioning through URIs
    std::string docRoot{"/vss/api/v1/"};

    auto pubKeyFile = variables["cert-path"].as<boost::filesystem::path>()/"jwt.key.pub";
    string jwtPubkey=Authenticator::getPublicKeyFromFile(pubKeyFile.string(), logger);
    if (jwtPubkey == "" ) {
        logger->Log(LogLevel::ERROR, "Could not read valid JWT pub key. Terminating.");
        return -1;
    }

    auto rest2JsonConverter =
        std::make_shared<RestV1ApiHandler>(logger, docRoot);
    auto httpServer = std::make_shared<WebSockHttpFlexServer>(
        logger, std::move(rest2JsonConverter));

    auto tokenValidator =
        std::make_shared<Authenticator>(logger, jwtPubkey, "RS256");
    auto accessCheck = std::make_shared<AccessChecker>(tokenValidator);

    auto  mqttClient = std::make_shared<MQTTClient>(
            logger, "vss", variables["mqtt.address"].as<string>()
            , variables["mqtt.port"].as<int>()
            , variables["mqtt.insecure"].as<bool>()
            , variables["mqtt.keepalive"].as<int>()
            , variables["mqtt.qos"].as<int>()
            , variables["mqtt.retry"].as<int>()
            );
    if (variables.count("mqtt.username")) {
        std::string password;
        if (!variables.count("mqtt.password")){
            std::cout << "Please input your mqtt password:" << std::endl;
            std::cin >> password;

        } else {
            password = variables["mqtt.password"].as<string>();
        }
        mqttClient->setUsernamePassword(variables["mqtt.username"].as<string>(), password);
    }
    if (variables.count("mqtt.topic-prefix")) {
        mqttClient->addPrefix(variables["mqtt.topic-prefix"].as<string>());
    }

    auto subHandler = std::make_shared<SubscriptionHandler>(
        logger, httpServer, mqttClient, tokenValidator, accessCheck);
    auto database =
        std::make_shared<VssDatabase>(logger, subHandler, accessCheck);
    auto cmdProcessor = std::make_shared<VssCommandProcessor>(
        logger, database, tokenValidator, subHandler);

    gDatabase = database.get();

    database->initJsonTree(vss_path);

    if(variables.count("mqtt.publish")){
        string path_to_publish = variables["mqtt.publish"].as<string>();

        // remove whitespace and "
        path_to_publish = std::regex_replace(path_to_publish, std::regex("\\s+"), std::string(""));
        path_to_publish = std::regex_replace(path_to_publish, std::regex("\""), std::string(""));

        if(!path_to_publish.empty()){
            std::stringstream topicsstream(path_to_publish);
            std::string token;
            while (std::getline(topicsstream, token, ';')) {
                if(database->checkPathValid(token)){
                    mqttClient->addPublishPath(token);
                }else{
                    logger->Log(LogLevel::ERROR, string("main: ") +
                    token + string(" is not a valid path to publish"));
                }
            }
        }
    }

    httpServer->AddListener(ObserverType::ALL, cmdProcessor);
    httpServer->Initialize(variables["address"].as<string>(), port,
                           std::move(docRoot),
                           variables["cert-path"].as<boost::filesystem::path>().string(), insecure);
    httpServer->Start();

    while (1) {
      usleep(1000000);
    }

  } catch (const program_options::error &ex) {
    print_usage(argv[0], desc);
    cerr << ex.what() << std::endl;
    return -1;
  } catch (const std::runtime_error &ex) {
    logger->Log(LogLevel::ERROR, "Fatal runtime error: "+ string(ex.what()));
    return -1;
  }
  return 0;
}
