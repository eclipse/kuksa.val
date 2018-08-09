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

#include "vss.hpp"
#include "vsscommandprocessor.hpp"
#include <iostream>
#include <sstream>
#include <stdint.h>
#include "vssserializer.hpp"
#include "vssdatabase.hpp"
#include "server_ws.hpp"
#include "visconf.hpp"

using namespace std;

extern struct node mainNode;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
extern WsServer server;
extern UInt32 subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];
void sendMessageToClient (string message, uint32_t subID);

int addSubscription (char* path , UInt32 subId) {

  struct signal foundSigs[MAX_SIGNALS];
  int clientID = subId/CLIENT_MASK;  
  int res =  getSignalsFromTree(&mainNode, path, foundSigs);
 
  if(res < 0) {
    return -1;
  }

  if (res > 1) {
      cout <<res <<"signals found in path" << path <<". Subscribe works for 1 signal at a time" << endl;
      return -2;
  }

  cout<< "Found 1 signal at "<< path <<endl;

  int sigId = foundSigs[0].id;

   if(subscribeHandle[sigId][clientID] != 0) {
      cout <<"Updating the previous subscribe ID with a new one"<< endl;
   }
   setSubscribeCallback(&sendMessageToClient);
   subscribeHandle[sigId][clientID] = subId;
   return 0;
}


int removeSubscription (UInt32 subId) {
   int clientID = subId/CLIENT_MASK;
    for(int i=0 ;i < MAX_SIGNALS ; i++) {
       if(subscribeHandle[i][clientID] == subId) {
               subscribeHandle[i][clientID] = 0;
               break;
        } 

    }
    return 0;
}

void removeAllSubscriptions (UInt32 clientID) {
   for(int i=0 ;i < MAX_SIGNALS ; i++) {
          subscribeHandle[i][clientID] = 0;
   } 
   cout <<"Removed all subscriptions for client with ID ="<< clientID*CLIENT_MASK << endl;
}

// call back method - do not change signature.
void sendMessageToClient (string message, uint32_t subID) {
    
    uint32_t connectionID = (subID/CLIENT_MASK)*CLIENT_MASK;
 
    //cout << " sending sub message to client "<<endl;
    auto send_stream = make_shared<WsServer::SendStream>();
    *send_stream << message;
    for(auto &a_connection : server.get_connections()) {
      if(a_connection->connectionID == connectionID){
         a_connection->send(send_stream);
         return;
      }
    }
   cout <<"No connection available with conn ID ="<<connectionID<<"Therefore trying to remove the subscription associated with the connection!"<<endl; 
   int res = removeSubscription (subID);
   if(res == 0) {
      cout<<"Removed the sub ID " << subID <<endl;
   } else {
      cout << "could not remove the sub ID " << subID <<endl;
   }
}


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


/** check if requestmalformed. Returns empty string if it is fine, or error JSON */
//string checkRequest(const string action, const json query) {
	/*if ( !vssPathExists("path",query) || !vssPathExists("requestId",query)) {
		int32_t request_id=query.get<int32_t>("request_id",-1);
		return malFormedRequestResponse(request_id,action);
	}
		
	std::string path = query.get<string>("path");
	if (!vssPathExists(path) ) {
		int32_t request_id=query.get<int32_t>("requestId",-1);
		return pathNotFoundResponse(request_id,action,path);
	}*/
		
//	return "";
//}

string getValueAsString(struct signal foundSig) {

  char* value_type = foundSig.value_type;
  void* value = foundSig.value;
  pthread_mutex_t mutex = foundSig.mtx;
  

  if(strcmp( value_type , "UInt8") == 0 ){
    pthread_mutex_lock(&mutex);
    auto s = std::to_string(*((UInt8*)value));
    pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "UInt16") == 0){
    pthread_mutex_lock(&mutex);
    auto s = std::to_string(*((UInt16*)value));
    pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "UInt32") == 0){
    pthread_mutex_lock(&mutex);
    auto s = std::to_string(*((UInt32*)value));
    pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "Int8") == 0){
    pthread_mutex_lock(&mutex);
    auto s = std::to_string(*((Int8*)value));
    pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "Int16") == 0){
     pthread_mutex_lock(&mutex);
     auto s = std::to_string(*((Int16*)value));
     pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "Int32") == 0){
     pthread_mutex_lock(&mutex);
     auto s = std::to_string(*((Int32*)value));
     pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "Float") == 0){
     pthread_mutex_lock(&mutex);
     auto s = std::to_string(*((Float*)value));
     pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "Double") == 0){
     pthread_mutex_lock(&mutex);
     auto s = std::to_string(*((Double*)value));
     pthread_mutex_unlock(&mutex);
    return s;
  }else if (strcmp( value_type , "Boolean") == 0){
     pthread_mutex_lock(&mutex);
     Boolean boolVal = *((Boolean*)value);
     string s;
     if(boolVal) {
         s = "true";
     }else {
         s = "false";
     }
     pthread_mutex_unlock(&mutex);
     return s;
  }else if (strcmp( value_type , "String") == 0){
    pthread_mutex_lock(&mutex);
    auto s = string(((char*)value));
    pthread_mutex_unlock(&mutex);
    return s;
   }else {
     printf(" The value type %s is not supported \n", value_type);
     return NULL;
  }

}

string jprocessGet(uint32_t request_id, string path) {

   json answer;
   answer["action"] = "get";
   answer["requestId"] = request_id;

   json res = getSignal(path);

   string val = res[path].as<string>();

   answer["value"] = val;
   answer["timestamp"]= time(NULL);
   
   stringstream ss; 
   ss << pretty_print(answer);
   return ss.str();
}



string processGet(uint32_t request_id, string path) {
    
       cout<< "path received from client"<< path<<endl;
        //UGLY but getSignalFromTree does not work with const
	const char *path_c=path.c_str();
	char * nonconst=strdup(path_c);
        
        struct signal foundSig[MAX_SIGNALS];
        int res = getSignalsFromTree(&mainNode, nonconst, foundSig);
        
	//pt::ptree res= vssGetSignalFromTree(path);
	/*int32_t res = getSignalFromTree(&main_node, nonconst, &foundSig);
        string response;*/
       if(res < 0) {
           string response;
           cout << " error num = "<< res << endl;
           json root;
           json error;

	   root["action"] = "get";
	   root["requestId"] = request_id;

           error["number"] = 404;
           error["reason"] =  "invalid_path";
           error["message"] =  "The specified data path does not exist.";


	   root["error"] = error;
	   root["timestamp"] = time(NULL);

	   stringstream ss; 
           ss << pretty_print(root);
           return ss.str();
     }
  
   json answer;
   answer["action"] = "get";
   answer["requestId"] = request_id;
   if(res > 1) {
      json children;
      for(int i=0 ; i < res ; i++) {
         json child;
         string val = getValueAsString(foundSig[i]);
         auto sigName = std::string(foundSig[i].signal_full_name);
         child[sigName] = val;
         children[""] = child;
      }
      answer["value"] = children;
   } else if (res == 1) {
      string val = getValueAsString(foundSig[0]);
      answer["value"]= val;
   }
   answer["timestamp"]= time(NULL);
   
   stringstream ss; 
   ss << pretty_print(answer);
   return ss.str();
}

string processSet(uint32_t request_id, string path, string value) {

  cout<< "path received from client"<< path <<endl;

   const char *path_c=path.c_str();
   char * nonconst=strdup(path_c);

   // get signals from vss tree.
   struct signal foundSig[MAX_SIGNALS];
   int res = getSignalsFromTree(&mainNode, nonconst, foundSig);
   
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

   // TODO handle also multiple signals.
   if(res == 1) {
      printf(" found the signal now trying to set\n");
       
       int sigID = foundSig[0].id;
       char* value_type = foundSig[0].value_type;
       int result = 0;
       string type = string(value_type);
       if(type == "UInt8" || type == "UInt16" || type == "UInt32" || type == "Int32" ) {
          uint64_t ul = std::strtoul (value.c_str() ,nullptr, 10);
          setValue(sigID , &ul);  
       } else if ( type == "Int8" || type == "Int16"  ) {
          int32_t ival = atoi(value.c_str());
          setValue(sigID , &ival);  
       } else if (type == "Double") {
          Double dval = strtod(value.c_str(), NULL);
          setValue(sigID , &dval); 
       } else if (type == "Float" ) {
          Float fval = atof(value.c_str());
          setValue(sigID , &fval); 
       }  else if (type == "Boolean") {
          Boolean val_bool;
             if(value == "true"){
                val_bool = true;
                setValue(sigID , &val_bool);
             } else if (value == "false") {
                val_bool = false;
                setValue(sigID , &val_bool);
             }
                
       } else { 
          setValue(sigID , (void*)value.c_str());
       }
       
       //if(result == 0) {
          json answer;
          answer["action"] ="set";
          answer["requestId"] = request_id;
          answer["timestamp"] = time(NULL);
   
          std::stringstream ss;
          ss << pretty_print(answer);
        return ss.str();
       // }
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

string processSubscribe(uint32_t request_id, string path, uint32_t connectionID) {
    cout<< "path received from client for subscription"<< path<<endl;
    // generate subscribe ID "randomly".
    uint32_t subId = rand() % 9999999;
    // embed connection ID into subID.
    subId = connectionID + subId;
#ifdef WITHCSV
    const char *path_c=path.c_str();
    char * nonconst=strdup(path_c);
    int res = addSubscription (nonconst , subId);
#else
    int res = jAddSubscription (path , subId);
    jSetSubscribeCallback(&sendMessageToClient);
#endif

    if( res == 0) {
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
      
        if(res == -1) {
           error["reason"] =  "invalid_path";
           error["message"] = "The specified data path does not exist.";
        }else if (res == -2) {
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

string processUnsubscribe(uint32_t request_id, uint32_t subscribeID) {

   int res = removeSubscription (subscribeID);
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

string processGetMetaData(int32_t request_id, string path) {
	json st = getMetaData(path);
	//cleanDataFromTree(&st);
	
	json result;
	result["action"] ="getMetadata";
        result["requestId"] = request_id; 
        result["metadata"]= st;
	result["timestamp"]= time(NULL);
	
	std::stringstream ss;
   	ss << pretty_print(result);
   	
	return ss.str(); 
}



 

string processQuery(string req_json , uint32_t connectionID) {

	json root;

        string response;

        root = json::parse(req_json);
	
	string action = root["action"].as<string>();
	
	if (action == "get")  {
		string path = root["path"].as<string>();
		uint32_t request_id = root["requestId"].as<int>();
		cout << "get query  for " << path << " with request id " <<  request_id << endl;
#ifdef WITHCSV
		response = processGet(request_id, path);
#else
                response = jprocessGet(request_id, path);
#endif

	} else if(action == "set")  {
		string path = root["path"].as<string>();
		uint32_t request_id = root["requestId"].as<int>();
		string value = root["value"].as<string>();
		cout << "set query  for " << path << " with request id " <<  request_id << endl;
		response = processSet(request_id, path , value);
       	} else if(action == "subscribe")  {
		string path = root["path"].as<string>();
		uint32_t request_id = root["requestId"].as<int>();
		cout << "subscribe query  for " << path << " with request id " <<  request_id << endl;
		response = processSubscribe(request_id, path , connectionID);
       	} else if(action == "unsubscribe")  {
		uint32_t request_id = root["requestId"].as<int>();
                uint32_t subscribeID = root["subscriptionId"].as<int>();
		cout << "unsubscribe query  for sub ID = " << subscribeID << " with request id " <<  request_id << endl;
		response = processUnsubscribe(request_id, subscribeID);
       	}else if (action == "getMetadata") {
		/*string resp = checkRequest(action,root);
		if (resp != "") {
			return resp;
		}*/
		uint32_t request_id = root["requestId"].as<int>();
		string path = root["path"].as<string>();
		cout << "metadata query  for " << path << " with request id " <<  request_id << endl;
		response = processGetMetaData(request_id,path);
	}  else {
		cout << "Unknown action " << action << endl;
	}

   return response;

}



