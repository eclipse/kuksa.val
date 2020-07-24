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

static void handle_method_call(GDBusConnection *connection, const gchar *msender,
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
  string configFile;
  vector<string> logLevels{"NONE"};
  uint8_t logLevelsActive = static_cast<uint8_t>(LogLevel::NONE);

  std::cout << "kuksa.val server" << std::endl;
  std::cout << "Commit "  << string(GIT_HEAD_SHA1).substr(0,7);
  if (GIT_IS_DIRTY)
    std::cout << "-dirty";
  std::cout << " from " << GIT_COMMIT_DATE_ISO8601 << std::endl;

  program_options::options_description desc{"Options"};
  desc.add_options()("help,h", "Help screen")(
      "config-file,cfg", program_options::value<string>(&configFile),
      "Configuration file path for program parameters. "
      "Can be provided instead of command line options")(
      "vss", program_options::value<string>(), "vss_rel*.json file")(
      "cert-path", program_options::value<string>()->default_value("."),
      "Path to directory where 'Server.pem' and 'Server.key' are located")(
      "insecure", "Accept plain (no-SSL) connections")(
      "use-keycloak", "Use KeyCloak for permission management")(
      "address", program_options::value<string>()->default_value("localhost"),
      "Address")("port", program_options::value<int>()->default_value(8090),
                 "Port")(
      "log-level",
      program_options::value<vector<string>>(&logLevels)->composing(),
      "Log level event type to be enabled. "
      "To enable different log levels, provide this option multiple times with "
      "required log levels. \n"
      "Supported log levels: NONE, VERBOSE, INFO, WARNING, ERROR, ALL");

  program_options::variables_map variables;
  program_options::store(parse_command_line(argc, argv, desc), variables);
  program_options::notify(variables);
  // if config file passed, get configuration from it
  if (configFile.size()) {
    cout << configFile << std::endl;
    std::ifstream ifs(configFile.c_str());
    if (!ifs) {
      std::cerr << "Could not open config file: " << configFile << std::endl;
      return -1;
    }
    program_options::store(parse_config_file(ifs, desc), variables);
  }
  program_options::notify(variables);

  // verify parameters

  if (!variables.count("vss")) {
    print_usage(argv[0], desc);
    cerr << "the option '--vss' is required but missing" << std::endl;
    return -1;
  }
  if (!variables.count("cert-path")) {
    cerr << "the option '--cert-path' is required but missing" << std::endl;
    print_usage(argv[0], desc);
    return -1;
  }
  if (variables.count("help")) {
    print_usage(argv[0], desc);
    return -1;
  }

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
    auto secure = !variables.count("insecure");
    auto vss_filename = variables["vss"].as<string>();

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

    string jwtPubkey=Authenticator::getPublicKeyFromFile(variables["cert-path"].as<string>()+"/jwt.key.pub",logger);
    if (jwtPubkey == "" ) {
        logger->Log(LogLevel::ERROR, "Could not read valid JWT pub key. Terminating.");
        return -1;
    }

    auto rest2JsonConverter =
        std::make_shared<RestV1ApiHandler>(logger, docRoot);
    auto httpServer = std::make_shared<WebSockHttpFlexServer>(
        logger, std::move(rest2JsonConverter));

    auto mqttClient = std::make_unique<MQTTClient>(
        logger, "vss", "localhost", 1883);

    auto tokenValidator =
        std::make_shared<Authenticator>(logger, jwtPubkey, "RS256");
    auto accessCheck = std::make_shared<AccessChecker>(tokenValidator);
    auto subHandler = std::make_shared<SubscriptionHandler>(
        logger, httpServer, std::move(mqttClient), tokenValidator, accessCheck);
    auto database =
        std::make_shared<VssDatabase>(logger, subHandler, accessCheck);
    auto cmdProcessor = std::make_shared<VssCommandProcessor>(
        logger, database, tokenValidator, subHandler);

    gDatabase = database.get();
    database->initJsonTree(vss_filename);

    httpServer->AddListener(ObserverType::ALL, cmdProcessor);
    httpServer->Initialize(variables["address"].as<string>(), port,
                           std::move(docRoot),
                           variables["cert-path"].as<string>(), !secure);
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
