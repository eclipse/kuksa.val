#ifndef __AUTHENTICATOR_H__
#define __AUTHENTICATOR_H__

#include <stdio.h>
#include "wschannel.hpp"

using namespace std;


class authenticator {


public:

   int validate (wschannel &channel , string authToken);
   bool isStillValid (wschannel &channel);

};
#endif
