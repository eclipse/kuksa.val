#ifndef __SUBSCRIPTIONHANDLER_H__
#define __SUBSCRIPTIONHANDLER_H__

#include <stdio.h>
#include <queue>
#include "visconf.hpp"
#include "wsserver.hpp"
#include "authenticator.hpp"
#include "vssdatabase.hpp"


using namespace std;


class subscriptionhandler {

 private:
   uint32_t subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];
   class wsserver* server;
   authenticator* validator;
   bool threadRun;
   
   

 public:

   
   queue<pair<uint32_t, string>> buffer; 
   subscriptionhandler(class wsserver* server, class authenticator* authenticate);
   uint32_t subscribe (class vssdatabase* db, uint32_t channelID  , string path);
   int unsubscribe (uint32_t subscribeID);
   int unsubscribeAll (uint32_t connectionID);
   int update ( int signalID, string value);
   int update ( string path, string value);
   class wsserver* getServer();
   int startThread();
   int stopThread();
   bool isThreadRunning(); 
};
#endif
