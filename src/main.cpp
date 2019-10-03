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

#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>

#include <boost/program_options.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/json_query.hpp>

#include "WsServer.hpp"
#include "VssDatabase.hpp"
#include "exception.hpp"

#include "BasicLogger.hpp"


using namespace std;
using namespace boost;
using namespace jsoncons;
using namespace jsoncons::jsonpath;
using jsoncons::json;


// Websocket port
#define PORT 8090

VssDatabase* database = NULL;

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


static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  (void) connection;
  (void) object_path;
  (void) interface_name;
  (void) parameters;
  (void) user_data;
  (void) sender;
  
  json jsonVal;
  const gchar *vss_path;
   
   if (database == NULL) {
      g_dbus_method_invocation_return_error (invocation,
                                                 G_IO_ERROR,
                                                 G_IO_ERROR_FAILED_HANDLED,
                                                 "No database instance yet.");
      return;
   }

   if (g_strcmp0 (method_name, "pushUnsignedInt") == 0)
   {
      guint64  value;
      g_variant_get (parameters, "(&st)", &vss_path, &value);
      jsonVal = value;
      g_message("pushUnsignedInt called with param ( %s , %ld )", vss_path, value);
   }
   else if (g_strcmp0 (method_name, "pushInt") == 0)
   {
     gint64  value;
     g_variant_get (parameters, "(&sx)", &vss_path, &value);
     jsonVal = value;
     g_message("PushInt called with param ( %s , %ld )", vss_path, value);
   }
   else if (g_strcmp0 (method_name, "pushDouble") == 0)
   {
     gdouble  value;
     g_variant_get (parameters, "(&sd)", &vss_path, &value);
     jsonVal = value;
     g_message("PushDouble called with param ( %s , %f )", vss_path, value);
   }

   else if (g_strcmp0 (method_name, "pushBool") == 0)
   {
     gboolean  value;
     g_variant_get (parameters, "(&sb)", &vss_path, &value);
     jsonVal = value;
     g_message("pushBool called with param ( %s , %d )", vss_path, value);

   } 

   else if (g_strcmp0 (method_name, "pushString") == 0)
   {
     const gchar *value;
     g_variant_get (parameters, "(&ss)", &vss_path, &value);
     jsonVal = std::string(value);
     g_message("pushString called with param ( %s , %s )", vss_path, value);

   }  
   else 
   {
      g_dbus_method_invocation_return_error (invocation,
                                                 G_IO_ERROR,
                                                 G_IO_ERROR_FAILED_HANDLED,
                                                 "Handling needs to be implemented here.");
   }

      // set the data in the db.
      try {
        string pathStr(vss_path);
        database->setSignal(pathStr, jsonVal);
      } catch (genException &e) {
        g_dbus_method_invocation_return_error (invocation,
                                                 G_IO_ERROR,
                                                 G_IO_ERROR_FAILED_HANDLED,
                                                 "Exception occured while setting data to the server.");
      }

      g_dbus_method_invocation_return_value (invocation, NULL);  


}

/* for now */
static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL,
  /* Padding for future expansion */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
  
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  (void) name;
  (void) user_data;
  guint registration_id;
  registration_id = g_dbus_connection_register_object (connection,
                                                       "/org/eclipse/kuksaW3Cbackend",
                                                       introspection_data->interfaces[0],
                                                       &interface_vtable,
                                                       NULL,
                                                       NULL,  /* user_data_free_func */
                                                       NULL); /* GError** */
  g_assert (registration_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
   (void) user_data;
   (void) connection;
   printf("Bus Name acquired %s", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  (void) user_data;
  (void) connection;
  (void) name;
  exit (1);
}


static void print_usage(const char *prog_name,
                        program_options::options_description desc) {
  cerr << "Usage: " << prog_name << " OPTIONS" << endl;
  cerr << desc << std::endl;
}

int main(int argc, const char *argv[]) {
  program_options::options_description desc{"Options"};
  desc.add_options()
    ("help,h", "Help screen")
    ("vss", program_options::value<string>(), "vss_rel*.json file")
    ("insecure", "Run insecure")
    ("port", program_options::value<int>()->default_value(8090), "Port")
    ("address", program_options::value<string>()->default_value("localhost"), "Address");

  try {
    program_options::variables_map variables;
    program_options::store(parse_command_line(argc, argv, desc), variables);
    program_options::notify(variables);

    if (!variables.count("vss")) {
      print_usage(argv[0], desc);
      cerr << "vss file (--vss) must be specified" << std::endl;
      return -1;
    }

    if (variables.count("help")) {
      print_usage(argv[0], desc);
      return -1;
    }
    auto port = variables["port"].as<int>();
    auto secure = !variables.count("insecure");
    auto vss_filename = variables["vss"].as<string>();

    // Start D-Bus backend connection.
    guint owner_id;
    GMainLoop *loop;

    introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
    g_assert (introspection_data != NULL);
  

    owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
                               "org.eclipse.kuksa.w3cbackend",
                               G_BUS_NAME_OWNER_FLAGS_NONE,
                               on_bus_acquired,
                               on_name_acquired,
                               on_name_lost,
                               NULL,
                               NULL);


    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);
    g_bus_unown_name (owner_id);
    g_dbus_node_info_unref (introspection_data);

    uint8_t logLevelsActive;
#ifdef DEBUG
    logLevelsActive = static_cast<uint8_t>(LogLevel::ALL);
#else
    logLevelsActive = static_cast<uint8_t>(LogLevel::INFO & LogLevel::WARNING & LogLevel::ERROR);
#endif
    std::shared_ptr<ILogger> logger = std::make_shared<BasicLogger>(logLevelsActive);

    WsServer server(logger, port, vss_filename, secure);
    server.start();

  } catch (const program_options::error &ex) {
    print_usage(argv[0], desc);
    cerr << ex.what() << std::endl;
    return -1;
  }
  return 0;
}
