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
#include "subscriptionhandler.hpp"


pthread_mutex_t subMutex;


subscriptionhandler::subscriptionhandler(class wsserver* wserver, class authenticator* authenticate) {
   server = wserver;
   validator = authenticate;
   pthread_mutex_init (&subMutex, NULL);
   threadRun = true;
   startThread();
}


uint32_t subscriptionhandler::subscribe (class vssdatabase* db, uint32_t channelID, string path) {
 
    // generate subscribe ID "randomly".
    uint32_t subId = rand() % 9999999;
    // embed connection ID into subID.
    subId = channelID + subId;

   
    bool isBranch = false;
    string jPath = db->getVSSSpecificPath(path, isBranch);
    
    if(jPath == "") {
        return -1;
    } 

    int clientID = channelID/CLIENT_MASK;
    json resArray = json_query(db->vss_tree , jPath);

    if(resArray.is_array() && resArray.size() == 1) {

       json result = resArray[0];
       int sigID = result["id"].as<int>();
#ifdef DEBUG
       if(subscribeHandle[sigID][clientID] != 0) {
          cout <<"Updating the previous subscribe ID with a new one"<< endl;
       }
#endif
       subscribeHandle[sigID][clientID] = subId;
       return subId;        
     } else if(resArray.is_array()) {
       cout <<resArray.size()<<"signals found in path" << path <<". Subscribe works for 1 signal at a time" << endl;
       return -2;
     } else {
       cout <<" some error occured while adding subscription"<<endl;
       return -3;
     }
}
   
int subscriptionhandler::unsubscribe (uint32_t subscribeID ) {
  int clientID = subscribeID/CLIENT_MASK;
    for(int i=0 ;i < MAX_SIGNALS ; i++) {
       if(subscribeHandle[i][clientID] == subscribeID) {
               subscribeHandle[i][clientID] = 0;
               return 0;
        } 
     }
   return -1;
}

int subscriptionhandler::unsubscribeAll ( uint32_t connectionID) {
   for(int i=0 ;i < MAX_SIGNALS ; i++) {
          subscribeHandle[i][connectionID] = 0;
   }
#ifdef DEBUG 
   cout <<"Removed all subscriptions for client with ID ="<< connectionID*CLIENT_MASK << endl;
#endif
   return 0;
}

int subscriptionhandler::update (int signalID, string value) {

  for(int i=0 ; i< MAX_CLIENTS ; i++) {
    uint32_t subID = subscribeHandle[signalID][i];
     if( subID != 0) {
        pthread_mutex_lock (&subMutex);
        pair<uint32_t, string> newSub;
        newSub = std::make_pair (subID,value);
        buffer.push(newSub);
        pthread_mutex_unlock (&subMutex);
     }
  }
  return 0;
}

class wsserver* subscriptionhandler::getServer() {
    return server;
}
   
int subscriptionhandler::update (string path, string value) {
  return 0;
}

void* subThread(void * instance) {
    subscriptionhandler* handler = (subscriptionhandler*) instance;
#ifdef DEBUG
    cout << "Started Subscription Thread!"<<endl;
#endif
    while (handler->isThreadRunning()) {
          
       pthread_mutex_lock (&subMutex);
       if(handler->buffer.size() > 0) {

          pair<uint32_t, string> newSub = handler->buffer.front();
          handler->buffer.pop();
          pthread_mutex_unlock (&subMutex);

          uint32_t subID = newSub.first;
          string value = newSub.second;

          json answer;
          answer["action"] = "subscribe";
          answer["subscriptionId"] = subID;
          answer.insert_or_assign("value", value);
          answer["timestamp"] = time(NULL);

          stringstream ss;
          ss << pretty_print(answer);
          string message = ss.str();
  
          uint32_t connectionID = (subID/CLIENT_MASK) * CLIENT_MASK;
          handler->getServer()->sendToConnection(connectionID , message);
       }
        usleep(10000);
     }
     cout << "Subscription handler thread stopped running"<<endl;
}

int subscriptionhandler::startThread() {

    pthread_t subscription_thread;
    if(pthread_create(&subscription_thread, NULL, &subThread, this)) {
      cout << "Error creating subscription handler thread"<<endl;
      return 1;
    }
}

int subscriptionhandler::stopThread() {  
   threadRun = false;
}


bool subscriptionhandler::isThreadRunning() {
   return threadRun; 
} 
