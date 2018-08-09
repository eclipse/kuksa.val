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
#include <stdexcept>
#include "vssdatabase.hpp"
#include "vss.hpp"
#include "visconf.hpp"

using jsoncons::json;
extern UInt32 subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];
void (*jSubCallBack)(string, uint32_t);
json vss_tree;

void initJsonTree() {

    std::ifstream is("vss_rel_1.0.json");
    is >> vss_tree;
    cout << "VSS tree initialized using JSON file" << endl;
    is.close();
	
}

vector<string> getPathTokens(string path) {

    vector<string> tokens;
    char* path_char = (char *) path.c_str();
    char* tok = strtok ( path_char , ".");
 
    while ( tok != NULL) {
       tokens.push_back(string(tok));
       tok = strtok( NULL ,".");
    }

    return tokens;
}


string getVSSSpecificPath (string path, bool &isBranch) {

    vector<string> tokens = getPathTokens(path);
    int tokLength = tokens.size();
    string format_path = "$.";
    for(int i=0; i < tokLength; i++) {
      try {
         format_path = format_path + tokens[i];
       
         json res = json_query(vss_tree , format_path);
       
         string type = "";
         if(res.is_array()) {
	   type = res[0]["type"].as<string>();
	 } else {
           type = res["type"].as<string>();
         }

         if (type != "" && type == "branch") {             
             format_path = format_path + ".children.";
             isBranch = true;
         } else if (type != "") {
             isBranch = false;
             break;
         } else {
             cout << "Path is invalid !!" << endl;
             return ""; 
         }

      } catch (...) {
         cout << "Exception occured while querying JSON. Check Path!"<<endl;
         return ""; 
      }
    }
  
    json pathRes =  json_query(vss_tree , format_path, result_type::path);
    string jPath = pathRes[0].as<string>();
    return jPath;
}


vector<string> getVSSTokens(string path) {

    vector<string> tokens;

    char* path_char = (char *) path.c_str();
 
    char* tok = strtok ( path_char , "['");
    int count = 1;
    while ( tok != NULL) {

       string tokString = string(tok);
       
       if( count % 2 == 0) {
           tok = strtok( NULL ,"']");
           tokens.push_back(tokString);
       } else {
           tok = strtok( NULL ,"['");
       }
       
       count ++;
    }

    return tokens;
}


json getMetaData(string path) {
  
	cout <<"Meta Data Path =" << path << endl;

	string format_path = "$";

        bool isBranch = false;

        string jPath = getVSSSpecificPath(path, isBranch);

        if(jPath == "") {
            return NULL;
        }

        
        cout << jPath << endl;

        vector<string> tokens = getVSSTokens(jPath);

	int tokLength = tokens.size();

	json parentArray[16];
        string parentName[16];

        int parentCount = 0;
        json resJson;
	for(int i=0; i< tokLength; i++) {
	    
	   format_path = format_path + "."+ tokens[i] ;
           if( (i < tokLength-1) && (tokens[i] == "children")) {
               continue;
           }

	   json resArray = json_query(vss_tree , format_path);
	  	  
	   if(resArray.is_array() && resArray.size() == 1) {
              resJson = resArray[0];
                 if(resJson.has_key("description")) {
		    resJson.insert_or_assign("description", resJson["description"].as<string>());
		 }
 
                 if(resJson.has_key("type")) {
                    string type = resJson["type"].as<string>();
		    resJson.insert_or_assign("type", type);
                 }
	   } else {
                 // handle exception.
                 cout << "More than 1 Branch/ value found!. Path requested needs to be more refined" << endl;
                 return NULL;
           }

           // last element is the requested signal.
	   if ((i == tokLength-1) && (tokens[i] != "children")) {
	      json value;
	      value.insert_or_assign(tokens[i],resJson); 
	      resJson = value;
           }

	   parentArray[parentCount] = resJson;
           parentName[parentCount] = tokens[i];
           parentCount++;
	}

	json result = resJson;
        

	for(int i = parentCount - 2; i >= 0 ; i--) {
	     json element =  parentArray[i];
	     element.insert_or_assign("children", result);
	     json parent;
	     parent.insert_or_assign(parentName[i] , element);
	     result = parent;
	 }

       return result;
}

int setSignal(string path, void* value) {

    bool isBranch = false;
    string jPath = getVSSSpecificPath(path, isBranch);

    if(jPath == "") {
         cout <<"Path " << path <<" not available in the DB"<<endl;
         return -1;
    } else if (isBranch) {
         cout <<"Path " << path <<" is a branch, need a path to a signal to set data"<<endl;
         return -2;
    }

    json resArray = json_query(vss_tree , jPath);
    json  resJson;
    if(resArray.is_array() && resArray.size() == 1) {
        resJson = resArray[0];

    if(resJson.has_key("type")) {
          string value_type = resJson["type"].as<string>();

	  if( value_type == "UInt8") {
	     resJson.insert_or_assign("value", *((UInt8*)value));
	  }else if (value_type == "UInt16") {
	     resJson.insert_or_assign("value", *((UInt16*)value));
	  }else if (value_type == "UInt32") {
	     resJson.insert_or_assign("value", *((UInt32*)value));
	  }else if (value_type == "Int8") {
	     resJson.insert_or_assign("value", *((Int8*)value));
	  }else if (value_type == "Int16") {
	     resJson.insert_or_assign("value", *((Int16*)value));
	  }else if (value_type == "Int32") {
	     resJson.insert_or_assign("value", *((Int32*)value));
	  }else if (value_type == "Float") {
	    resJson.insert_or_assign("value", *((Float*)value));
	  }else if (value_type == "Double") {
	     resJson.insert_or_assign("value", *((Double*)value));
	  }else if (value_type == "Boolean") {
	     resJson.insert_or_assign("value", *((Boolean*)value));
	  }else if (value_type == "String") {
	     resJson.insert_or_assign("value", string((char*)value));
	  }else {
	     cout<< "The value type "<< value_type <<" is not supported"<< endl;
	     return -2;
	  }

        //TODO- add mutex here.
        
        json_replace(vss_tree , jPath, resJson);
        cout << " new value set at path " << path << endl;
        //-------------------------------------


        int signalID = resJson["id"].as<int>();
        for(int i=0 ; i < MAX_CLIENTS; i++) {
           if( subscribeHandle[signalID][i] != 0) {
              json answer;
              answer["action"] = "subscribe";
              answer["subscriptionId"] = subscribeHandle[signalID][i];
              answer.insert_or_assign("value", resJson["value"].as<string>());
              answer["timestamp"] = time(NULL);
              stringstream ss;
              ss << pretty_print(answer);
              jSubCallBack(ss.str(), subscribeHandle[signalID][i]);

           }
         }

      } else {
          cout << "Type key not found for " << jPath << endl;
      }
        
    } else if (resArray.is_array()) {
        cout <<"Path " << path <<" has "<<resArray.size()<<" signals, the path needs refinement"<<endl;
        return -3;
    }
   
    return 0;
}

json getSignal(string path) {
    
    bool isBranch = false;
    string jPath = getVSSSpecificPath(path, isBranch);

    if(jPath == "") {
        json answer;
        answer[path] = NULL;
        return answer;
    } 

    cout << " GET request for "<< jPath << endl;
    json resArray = json_query(vss_tree , jPath);

    if(resArray.is_array() && resArray.size() == 1) {
       json answer;
       json result = resArray[0];
       if(result.has_key("value")) {
          answer[path] = result["value"].as<string>();
          return answer;
       }else {
          answer[path] = NULL;
          return answer;
       }
    }   
    return NULL;
}

int jAddSubscription (string path , uint32_t subId ) {

    bool isBranch = false;
    string jPath = getVSSSpecificPath(path, isBranch);
    int clientID = subId/CLIENT_MASK;
    if(jPath == "") {
        return -1;
    } 

    json resArray = json_query(vss_tree , jPath);

    if(resArray.is_array() && resArray.size() == 1) {

       json result = resArray[0];
       int sigId = result["id"].as<int>();

       if(subscribeHandle[sigId][clientID] != 0) {
          cout <<"Updating the previous subscribe ID with a new one"<< endl;
       }
       
       subscribeHandle[sigId][clientID] = subId;
       return 0;        
     } else if(resArray.is_array()) {
       cout <<resArray.size()<<"signals found in path" << path <<". Subscribe works for 1 signal at a time" << endl;
       return -2;
     } else {
       cout <<" some error occured while adding subscription"<<endl;
       return -3;
     }

}

void jSetSubscribeCallback(void(*sendMessageToClient)(std::string, uint32_t)){
     jSubCallBack = sendMessageToClient;
}



