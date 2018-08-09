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

#ifndef __VSS_HPP__
#define __VSS_HPP__

#include <stdio.h> 
#include <stdbool.h> 
#include <pthread.h> 
#include <string>
#define NAME 0
const int ID=1;
#define TYPE 2
#define UNIT 3
#define MIN  4
#define MAX  5
#define DESC 6
#define POSSIBLE_VAL 7

typedef unsigned char UInt8 ; 
typedef unsigned short int UInt16 ;
typedef unsigned long int UInt32 ;
typedef signed char Int8 ;
typedef short int Int16 ;
typedef long int Int32 ;
typedef float Float ;
typedef double Double ;
typedef bool Boolean ;

 struct node;

 struct branch {
   
   char* branch_name;
   char* description;
   struct node* from;
   struct node* to;

};

 struct signal {
  
  char* signal_name;
  char* signal_full_name;
  char* description;
  char* value_type;
  void* value;
  char* unit;
  int id;
  char* possible_values;
  pthread_mutex_t mtx;
  struct node* node;

};


 struct node {

    char* node_name;
    struct branch** branches;
    struct signal** signals;
    int branch_size;
    int signal_size;
};

struct subscription {
  char* clientAddress;
  UInt32 subId;
}; 
 

void initVSS (struct node* mainNode , struct signal** ptrSignals);
int getSignalsFromTree(struct node* mainNode, char* path, struct signal sig[]);
int setValue( int signalID , void* value);
void setSubscribeCallback(void(*sendMessageToClient)(std::string, uint32_t));
#endif
