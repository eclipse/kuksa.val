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
#include "visconf.hpp"

vssdatabase::vssdatabase(class subscriptionhandler* subHandle) {
   subHandler = subHandle;
}

void vssdatabase::initJsonTree() {
    string fileName = "vss_rel_1.0.json";
    std::ifstream is(fileName);
    is >> vss_tree;
#ifdef DEBUG
    cout << "vssdatabase::vssdatabase : VSS tree initialized using JSON file = " << fileName << endl;
#endif
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


string vssdatabase::getVSSSpecificPath (string path, bool &isBranch) {

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
             cout << "vssdatabase::getVSSSpecificPath : Path is invalid !!" << endl;
             return ""; 
         }

      } catch (...) {
         cout << "vssdatabase::getVSSSpecificPath :Exception occured while querying JSON. Check Path!"<<endl;
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


json vssdatabase::getMetaData(string path) {

	string format_path = "$";
        bool isBranch = false;
        string jPath = getVSSSpecificPath(path, isBranch);

        if(jPath == "") {
            return NULL;
        }
#ifdef DEBUG
        cout << "vssdatabase::getMetaData: VSS specific path =" << jPath << endl;
#endif

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
                 cout << "vssdatabase::getMetaData : More than 1 Branch/ value found! Path requested needs to be more refined" << endl;
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

int vssdatabase::setSignal(string path, string value) {

    bool isBranch = false;
    string jPath = getVSSSpecificPath(path, isBranch);

    if(jPath == "") {
         cout <<"vssdatabase::setSignal: Path " << path <<" not available in the DB"<<endl;
         return -1;
    } else if (isBranch) {
         cout <<"vssdatabase::setSignal: Path " << path <<" is a branch, need a path to a signal to set data"<<endl;
         return -2;
    }

    json resArray = json_query(vss_tree , jPath);
    json  resJson;
    if(resArray.is_array() && resArray.size() == 1) {
        resJson = resArray[0];

    if(resJson.has_key("type")) {
          string value_type = resJson["type"].as<string>();

	  if( value_type == "UInt8") {
	     resJson.insert_or_assign("value", stoi(value));
	  }else if (value_type == "UInt16") {
	     resJson.insert_or_assign("value", stoi(value));
	  }else if (value_type == "UInt32") {
	     resJson.insert_or_assign("value", stoi(value));
	  }else if (value_type == "Int8") {
	     resJson.insert_or_assign("value", stoi(value));
	  }else if (value_type == "Int16") {
	     resJson.insert_or_assign("value", stoi(value));
	  }else if (value_type == "Int32") {
	     resJson.insert_or_assign("value", stoi(value));
	  }else if (value_type == "Float") {
	    resJson.insert_or_assign("value", stof(value));
	  }else if (value_type == "Double") {
	     resJson.insert_or_assign("value", stod(value));
	  }else if (value_type == "Boolean") {
             if( value == "true")
	        resJson.insert_or_assign("value", true);
             else
                resJson.insert_or_assign("value", false);
	  }else if (value_type == "String") {
	     resJson.insert_or_assign("value", value);
	  }else {
	     cout<< "vssdatabase::setSignal: The value type "<< value_type <<" is not supported"<< endl;
	     return -2;
	  }

        //TODO- add mutex here.
        
        json_replace(vss_tree , jPath, resJson);
#ifdef DEBUG
        cout << "vssdatabase::setSignal: new value set at path " << path << endl;
#endif
        //-------------------------------------

        int signalID = resJson["id"].as<int>();
        // TODO- check if possible without converting to string.
        string value = resJson["value"].as<string>();
        subHandler->update(signalID, value);

      } else {
          cout << "vssdatabase::setSignal: Type key not found for " << jPath << endl;
      }
        
    } else if (resArray.is_array()) {
        cout <<"vssdatabase::setSignal : Path " << path <<" has "<<resArray.size()<<" signals, the path needs refinement"<<endl;
        return -3;
    }
   
    return 0;
}


void setJsonValue(json& dest , json& source , string key) {

  if(!source.has_key("type")) {
     cout << "vssdatabase::setJsonValue : could not set value! type is not present in source json!"<< endl;
     return;
  }

  string type = source["type"].as<string>();

  if(type == "String") {
      dest[key] = source["value"].as<string>();
  } else if (type == "UInt8") {
      dest[key] = source["value"].as<uint8_t>();
  } else if (type == "UInt16") {
      dest[key] = source["value"].as<uint16_t>();
  } else if (type == "UInt32") {
      dest[key] = source["value"].as<uint32_t>();
  } else if (type == "Int8") {
      dest[key] = source["value"].as<int8_t>();
  } else if (type == "Int16") {
      dest[key] = source["value"].as<int16_t>();
  } else if (type == "Int32") {
      dest[key] = source["value"].as<int32_t>();
  } else if (type == "Float") {
      dest[key] = source["value"].as<float>();
  } else if (type == "Double") {
      dest[key] = source["value"].as<double>();
  } else if (type == "Boolean") {
      dest[key] = source["value"].as<bool>();
  } else {
      cout << "vssdatabase::setJsonValue : could not set value! type was unknown"<<endl;
  }
}

json vssdatabase::getSignal(string path) {
    
    bool isBranch = false;
    string jPath = getVSSSpecificPath(path, isBranch);

    if(jPath == "") {
        json answer;
        return answer;
    } 

#ifdef DEBUG
    cout << "vssdatabase::getSignal:GET request for "<< jPath << endl;
#endif
    json resArray = json_query(vss_tree , jPath);
    json answer;
    if(resArray.is_array() && resArray.size() == 1) {
       json result = resArray[0];
       if(result.has_key("value")) {
          setJsonValue(answer , result, "value");
          return answer;
       }else {
          answer["value"] = 0;
          return answer;
       }
    }
    return answer;
}
