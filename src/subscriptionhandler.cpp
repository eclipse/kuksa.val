#include "subscriptionhandler.hpp"



subscriptionhandler::subscriptionhandler(class wsserver* wserver, class authenticator* authenticate) {
   server = wserver;
   validator = authenticate;
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
     if( subscribeHandle[signalID][i] != 0) {
        json answer;
        uint32_t subID = subscribeHandle[signalID][i];
        answer["action"] = "subscribe";
        answer["subscriptionId"] = subID;
        answer.insert_or_assign("value", value);
        answer["timestamp"] = time(NULL);

        stringstream ss;
        ss << pretty_print(answer);
        string message = ss.str();

        uint32_t connectionID = (subID/CLIENT_MASK)*CLIENT_MASK;
        server->sendToConnection(connectionID , message);
        //cout << " sending sub message to client "<<endl;
        
       /* cout <<"No connection available with conn ID ="<<connectionID<<"Therefore trying to remove the subscription associated with the connection!"<<endl; 
        int res = removeSubscription (subID);
         if(res == 0) {
           cout<<"Removed the sub ID " << subID <<endl;
         } else {
           cout << "could not remove the sub ID " << subID <<endl;
         }*/
     }
  }
  return 0;
}
   
int subscriptionhandler::update (string path, string value) {
  return 0;
}
