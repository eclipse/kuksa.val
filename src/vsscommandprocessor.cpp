/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
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

#include "vsscommandprocessor.hpp"
#include <iostream>
#include <sstream>
#include <stdint.h>
#include "vssserializer.hpp"
#include "vssdatabase.hpp"
#include "server_ws.hpp"
#include "visconf.hpp"


using namespace std;

string malFormedRequestResponse(int32_t request_id, const string action) {
   json answer;
   answer["action"] = action;
   answer["requestId"]= request_id;
   json error;
   error["number"] = 400;
   error["reason"] = "Request malformed";
   error["message"] = "Request malformed";
   answer["error"] = error;
   answer["timestamp"] = time(NULL);
   stringstream ss; 
   ss << pretty_print(answer);
   return ss.str();
}

/** A API call requested a non-existant path */
string pathNotFoundResponse(int32_t request_id, const string action, const string path) {

   json answer;
   answer["action"] = action;
   answer["requestId"]= request_id;
   json error;
   error["number"] = 404;
   error["reason"] = "Path not found";
   error["message"] = "I can not find "+path+" in my db";
   answer["error"] = error;
   answer["timestamp"] = time(NULL);
   stringstream ss; 
   ss << pretty_print(answer);
   return ss.str();
}

vsscommandprocessor::vsscommandprocessor(class vssdatabase* dbase, class  authenticator* vdator , class subscriptionhandler* subhandler) {
   database = dbase;
   tokenValidator = vdator;
   subHandler = subhandler;
   accessValidator = new accesschecker(tokenValidator); 
}


string vsscommandprocessor::processGet(uint32_t request_id, string path) {

   json answer;
   answer["action"] = "get";
   answer["requestId"] = request_id;

   json res = database->getSignal(path);

   string val = res[path].as<string>();

   answer["value"] = val;
   answer["timestamp"]= time(NULL);
   
   stringstream ss; 
   ss << pretty_print(answer);
   return ss.str();
}


string vsscommandprocessor::processSet(uint32_t request_id, string path, string value) {

  cout<< "path received from client"<< path <<endl;

   const char *path_c=path.c_str();
   char * nonconst=strdup(path_c);
   int res = database->setSignal(path, value);
   
   if (res < 0) {
        json root;
        json error;

	root["action"] = "set";
	root["requestId"] = request_id;

        error["number"] = 404;
        error["reason"] = "invalid_path";
        error["message"] = "The specified data path does not exist.";


	root["error"] =  error;
	root["timestamp"] = time(NULL);

	std::stringstream ss;
	ss << pretty_print(root);
        return ss.str();
   }   

  
   if(res == 1) {
       // TODO handle also multiple signals. 
   } else if(res == 0) {
        json answer;
        answer["action"] ="set";
        answer["requestId"] = request_id;
        answer["timestamp"] = time(NULL);
   
        std::stringstream ss;
        ss << pretty_print(answer);
        return ss.str();
   } else {

        json root;
        json error;

	root["action"] = "set";
	root["requestId"] = request_id;

        error["number"] = 401;
        error["reason"] = "Unknown error";
        error["message"] = "Error while setting data";


	root["error"] = error;
	root["timestamp"] = time(NULL);

	std::stringstream ss;
	ss << pretty_print(root);
        return ss.str();
   } 
}

string vsscommandprocessor::processSubscribe(uint32_t request_id, string path, uint32_t connectionID) {

#ifdef DEBUG
    cout<< "path received from client for subscription"<< path<<endl;
#endif
   

      uint32_t subId = -1;
      subId = subHandler->subscribe(database, connectionID , path);


    if( subId > 0) {
       json answer;
       answer["action"] = "subscribe";
       answer["requestId"] = request_id;
       answer["subscriptionId"] = subId;
       answer["timestamp"] = time(NULL);
   
       std::stringstream ss;
       ss << pretty_print(answer);
       return ss.str();

    } else {

        json root;
        json error;

	root["action"] = "subscribe";
	root["requestId"] = request_id;
      
        if(subId == -1) {
           error["reason"] =  "invalid_path";
           error["message"] = "The specified data path does not exist.";
        }else if (subId == -2) {
           error["reason"] = "Too mamy signals";
           error["message"] = "The specified data path has more than 1 signal.";
        } else {
           error["reason"] = "unknown Error";
           error["message"] = "Unknown";
        }

	root["error"] = error;
	root["timestamp"] = time(NULL);

	std::stringstream ss;

	ss << pretty_print(root);
       return ss.str();

    }
}

string vsscommandprocessor::processUnsubscribe(uint32_t request_id, uint32_t subscribeID) {

   int res = -1;
   if( res == 0) {
       json answer;
       answer["action"] = "unsubscribe";
       answer["requestId"] = request_id;
       answer["subscriptionId"] = subscribeID;
       answer["timestamp"] = time(NULL);
   
       std::stringstream ss;
       ss << pretty_print(answer);
       return ss.str();

    } else {


       json root;
          json error;

	  root["action"] = "unsubscribe";
	  root["requestId"] = request_id;

          error["reason"] = "Unknown error";
          error["message"] = "Error while unsubscribing";


	  root["error"] = error;
	  root["timestamp"] = time(NULL);

	  std::stringstream ss;
	  ss << pretty_print(root);
          return ss.str();
    }

}

string vsscommandprocessor::processGetMetaData(uint32_t request_id, string path) {
	json st = database->getMetaData(path);
	
	json result;
	result["action"] ="getMetadata";
        result["requestId"] = request_id; 
        result["metadata"]= st;
	result["timestamp"]= time(NULL);
	
	std::stringstream ss;
   	ss << pretty_print(result);
   	
	return ss.str(); 
}

string vsscommandprocessor::processAuthorize (uint32_t request_id, string token ,class wschannel& channel) {


        int ttl = tokenValidator->validate(channel, token);

        if( ttl == -1) {
           json result;
	   result["action"] ="authorize";
           result["requestId"] = request_id; 
           result["error"]= "Invalid Token";
	   result["timestamp"]= time(NULL);
	
	   std::stringstream ss;
   	   ss << pretty_print(result);
	   return ss.str();

        } else {

           json result;
	   result["action"] ="authorize";
           result["requestId"] = request_id; 
           result["TTL"]= ttl;
	   result["timestamp"]= time(NULL);
	
	   std::stringstream ss;
   	   ss << pretty_print(result);
	   return ss.str();
        } 

}


string vsscommandprocessor::processQuery(string req_json , class wschannel& channel) {

	json root;
        string response;
        root = json::parse(req_json);
	string action = root["action"].as<string>();
	
        if ( action == "authorize") {
                string token = root["tokens"].as<string>();
                uint32_t request_id = root["requestId"].as<int>();
#ifdef DEBUG
		cout << "authorize query with token = " << token << " with request id " <<  request_id << endl;
#endif
                response = processAuthorize (request_id, token , channel); 
        } else if(action == "unsubscribe")  {
		   uint32_t request_id = root["requestId"].as<int>();
                   uint32_t subscribeID = root["subscriptionId"].as<int>();
#ifdef DEBUG
		   cout << "unsubscribe query  for sub ID = " << subscribeID << " with request id " <<  request_id << endl;
#endif
		   response = processUnsubscribe(request_id, subscribeID);
       	} else {
                string path = root["path"].as<string>();
                uint32_t request_id = root["requestId"].as<int>();
                bool hasAccess = accessValidator->checkAccess(channel , path);
                
                if (!hasAccess) {
                    json result;
	   	    result["action"] =action;
           	    result["requestId"] = request_id; 
           	    result["error"]= "Not Authorized. You need to authorize with a valid token!";
	   	    result["timestamp"]= time(NULL);
	
	            std::stringstream ss;
   	            ss << pretty_print(result);
	            return ss.str();

                }
                if (action == "get")  { 
#ifdef DEBUG
		   cout << "get query  for " << path << " with request id " <<  request_id << endl;
#endif
                   response = processGet(request_id, path);
	        } else if(action == "set")  {
		   string value = root["value"].as<string>();
#ifdef DEBUG
		   cout << "set query  for " << path << " with request id " <<  request_id  << "value " << value << endl;
#endif
		   response = processSet(request_id, path , value);
       	        } else if(action == "subscribe")  {
#ifdef DEBUG
		   cout << "subscribe query  for " << path << " with request id " <<  request_id << endl;
#endif
		   response = processSubscribe(request_id, path , channel.getConnID());
       	        }  else if (action == "getMetadata") {
#ifdef DEBUG
		   cout << "metadata query  for " << path << " with request id " <<  request_id << endl;
#endif
		   response = processGetMetaData(request_id,path);
	        }  else {
		   cout << "Unknown action " << action << endl;
	        }
        }

   return response;

}


