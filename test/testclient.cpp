/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH and others.
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

#include <jsoncons/json.hpp>
#include <boost/program_options.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;
using namespace jsoncons;
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;               // from <boost/asio/ssl.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

bool subThread = false;


string getRequest(string path ) {
  json req;
   req["requestId"] = 1234;
   req["action"]= "get";
   req["path"] = string(path);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();
}

string setRequest(string path, string val) {
  json req;
   req["requestId"] = 1235;
   req["action"]= "set";
   req["path"] = string(path);
   req["value"] = string(val);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();
}

string getMetarequest( string path ) {
  json req;
   req["requestId"] = 1236;
   req["action"]= "getMetadata";
   req["path"] = string(path);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();
}

string getSubRequest (string path) {
   json req;
   req["requestId"] = 1237;
   req["action"]= "subscribe";
   req["path"] = string(path);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();
}

string getAuthRequest(string token) {
  json req;
   req["requestId"] = 1238;
   req["action"]= "authorize";
   req["tokens"] = string(token);
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();
}

string getKuksaAuthRequest(string clientID , string secret) {

  json req;
   req["requestId"] = 1238;
   req["action"]= "kuksa-authorize";
   req["clientid"] = clientID;
   req["secret"] = secret;
   stringstream ss; 
   ss << pretty_print(req);
   return ss.str();

}

void LoadCertData(ssl::context& ctx) {
  std::string cert;
  std::string key;
  std::string line;

  {
    ifstream inputFile ("Client.pem");
    if (inputFile.is_open())
    {
      while ( getline (inputFile,line) )
      {
        cert.append(line);
        cert.append("\n");
      }
      inputFile.close();
    }
  }
  {
    ifstream inputFile ("Client.key");
    if (inputFile.is_open())
    {
      while ( getline (inputFile,line) )
      {
        key.append(line);
        key.append("\n");
      }
      inputFile.close();
    }
  }

  ctx.use_certificate_chain(
      boost::asio::buffer(cert.data(), cert.size()));

  ctx.use_private_key(
      boost::asio::buffer(key.data(), key.size()),
      boost::asio::ssl::context::file_format::pem);
}

void StartBeastClient(std::string host, int port, std::string rootDoc) {
  // The io_context is required for all I/O
  boost::asio::io_context ioc;


  tcp::resolver resolver{ioc};
  websocket::stream<tcp::socket> ws{ioc};

  // Look up the domain name
  auto const results = resolver.resolve(host, to_string(port));

  // Make the connection on the IP address we get from a lookup
  boost::asio::connect(ws.next_layer(), results.begin(), results.end());

  // Perform the websocket handshake
  ws.handshake(host, "/" + rootDoc);

  for(;;) {
    string path, function;
      cout << "Enter vss path eg : Vehicle.Drivetrain.Transmission.DriveType " <<endl;
      getline (cin, path);
      cout << "Enter vis Function eg: authorize, get, set, getmetadata "<< endl;
      getline (cin, function);
      string command;

      if(function == "get") 
      {
         command = getRequest(path);
      } 
      else if (function == "set") 
      {
         string val;
         // TODO: (TS )
         cout << "Enter an integer value for the path "<< endl;
         getline (cin, val);
         command = setRequest(path, val);
      } 
      else if (function == "getmetadata")
      {
          command = getMetarequest(path);
      } 
      else if (function == "authorize") 
      {
          string token;
          cout << "Enter Token "<< endl;
          getline (cin, token);
          command = getAuthRequest(token);
      }

    // Send the message
    ws.write(boost::asio::buffer(std::string(command)));

    // This buffer will hold the incoming message
    boost::beast::multi_buffer b;

    // Read a message into our buffer
    ws.read(b);

    // The buffers() function helps print a ConstBufferSequence
    std::cout << boost::beast::buffers(b.data()) << std::endl;
  }

  // Close the WebSocket connection
  ws.close(websocket::close_code::normal);
}

void StartSecuredBeastClient(std::string host, int port, std::string rootDoc) {
  // The io_context is required for all I/O
  boost::asio::io_context ioc;

  tcp::resolver resolver{ioc};

  // The SSL context is required, and holds certificates
  ssl::context ctx{ssl::context::sslv23_client};
  // This holds the root certificate used for verification
  LoadCertData(ctx);
  websocket::stream<ssl::stream<tcp::socket>> ws{ioc, ctx};

  // Look up the domain name
  auto const results = resolver.resolve(host, to_string(port));

  // Make the connection on the IP address we get from a lookup
  boost::asio::connect(ws.next_layer().next_layer(), results.begin(), results.end());

  // Perform the SSL handshake
  ws.next_layer().handshake(ssl::stream_base::client);

  // Perform the websocket handshake
  ws.handshake(host, "/" + rootDoc);

  for(;;) {
    string path, function;
      cout << "Enter vss path eg : Vehicle.Drivetrain.Transmission.DriveType " <<endl;
      getline (cin, path);
      cout << "Enter vis Function eg: authorize, get, set, getmetadata "<< endl;
      getline (cin, function);
      string command;
      if(function == "get") 
      {
         command = getRequest(path);
      }
      else if (function == "set") 
      {
         string val;
         cout << "Enter a value for the path "<< endl;
         getline (cin, val);
         command = setRequest(path, val);
      } 
      else if (function == "getmetadata") 
      {
          command = getMetarequest(path);
      } 
      else if (function == "authorize") 
      {
          string token;
          cout << "Enter Token "<< endl;
          getline (cin, token);
          command = getAuthRequest(token);
      }
    // Send the message
    ws.write(boost::asio::buffer(std::string(command)));

    // This buffer will hold the incoming message
    boost::beast::multi_buffer b;

    // Read a message into our buffer
    ws.read(b);

    // The buffers() function helps print a ConstBufferSequence
    std::cout << boost::beast::buffers(b.data()) << std::endl;
  }

  // Close the WebSocket connection
  ws.close(websocket::close_code::normal);
}

static void print_usage(const char *prog_name,
                        boost::program_options::options_description desc) {
  cerr << "Usage: " << prog_name << " OPTIONS" << endl;
  cerr << desc << std::endl;
}

int main(int argc, const char *argv[]) {
  boost::program_options::options_description desc{"Options"};
  desc.add_options()
    ("help,h", "Help screen")
    ("insecure", "Run insecure plain Web-socket mode, available only in non wss-client mode; Default: SSL Web-socket mode")
    ("address", boost::program_options::value<string>()->default_value("localhost"), "Address")
    ("port", boost::program_options::value<int>()->default_value(8090), "Port");

  try {
    boost::program_options::variables_map variables;
    boost::program_options::store(parse_command_line(argc, argv, desc), variables);
    boost::program_options::notify(variables);

    if (variables.count("help")) {
      print_usage(argv[0], desc);
      return 0;
    }

    if (variables.count("insecure") > 0)
    {
      std::cout << "Starting Plain Boost.Beast WebSocket client" << std::endl;
      StartBeastClient(variables["address"].as<string>(),
                        variables["port"].as<int>(),
                        "vss");
    }
    else
    {
      std::cout << "Starting Secured Boost.Beast WebSocket client" << std::endl;
      StartSecuredBeastClient(variables["address"].as<string>(),
                              variables["port"].as<int>(),
                              "vss");
    }
  } catch (const boost::program_options::error &ex) {
    print_usage(argv[0], desc);
    cerr << ex.what() << std::endl;
    return -1;
  }
}
