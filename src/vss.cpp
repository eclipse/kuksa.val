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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vss.hpp"
#include "visconf.hpp"
#include "vssdatabase.hpp"
#include <boost/tokenizer.hpp>

using namespace std;

extern struct signal* ptrToSignals[MAX_SIGNALS];
extern struct node mainNode;
extern UInt32 subscribeHandle[MAX_SIGNALS][MAX_CLIENTS];
void (*subCallBack)(string, uint32_t);



// Util method that returns the child of "to" node based on the branch names passed.
struct node* getNode(struct node* parentNode, char** branchNames , int len) {

    if (parentNode->branch_size > 0) {
        // init return value to parent node.
        struct node * to_node = parentNode;
        for(int j=0 ; j<len ; j++){
           for(int i=0 ; i < to_node->branch_size; i++ ) {
               if(strcmp(branchNames[j], to_node->branches[i]->branch_name)== 0) {
                   to_node = to_node->branches[i]->to;
                   break;
               }
           }
        }
        return to_node;
    }
}

// Util method that returns the length of the string.
int getLength (char * line) {

    int i=0;
    if(line != NULL){
        while((line[i] != '\0') && (line[i] != '\r') &&(line[i] != '\n')){
            i++;
        }
    }
    return i;
}

// Util method that tokenizes the line read from the input csv file.
void getfields(char* line, char** fields) {
  typedef boost::tokenizer<boost::escaped_list_separator<char>> tokenizer;
  string lineString(line);

  tokenizer tok{lineString};
  int i = 0;
  for(tokenizer::iterator it= tok.begin(); it != tok.end() ; ++it) {
     fields[i++] = strdup((char*)((*it).c_str()));
  }
}


// Tokenizes all the elements in the path.
void getElements(char * path , char** elements, int* len) {

  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  string pathString(path);
  boost::char_separator<char> sep {"."};
  tokenizer tok{pathString, sep};
  int i = 0;
  for(tokenizer::iterator it= tok.begin(); it != tok.end() ; ++it) {
     elements[i++] = strdup((char*)((*it).c_str()));
  }
   *len = i;
}


// Util method for assigning memory for signal value field.
void allocateValueSpace(struct signal * sig) {

    if(strcmp(sig->value_type,"UInt8") == 0){
        sig->value  = (UInt8*) malloc(sizeof(UInt8));
    } else if(strcmp(sig->value_type,"UInt16") == 0){
        sig->value  = (UInt16*) malloc(sizeof(UInt16));
    } else if(strcmp(sig->value_type,"UInt32") == 0){
        sig->value  = (UInt32*) malloc(sizeof(UInt32));
    } else if(strcmp(sig->value_type,"Int8") == 0){
        sig->value  = (Int8*) malloc(sizeof(Int8));
    } else if(strcmp(sig->value_type,"Int16") == 0){
        sig->value  = (Int16*) malloc(sizeof(Int16));
    } else if(strcmp(sig->value_type,"Int32") == 0){
        sig->value  = (Int32*) malloc(sizeof(Int32));
    } else if(strcmp(sig->value_type,"Float") == 0){
        sig->value  = (Float*) malloc(sizeof(Float));
    } else if(strcmp(sig->value_type,"Double") == 0){
        sig->value  = (Double*) malloc(sizeof(Double));
    } else if(strcmp(sig->value_type,"Boolean") == 0){
        sig->value  = (Boolean*) malloc(sizeof(Boolean));
    }
}

// Creates the VSS signal tree. TODO -  Differentiate between signal and attribute.
void createTree(char** fields , struct node * main_node , struct signal** ptrSignals) {

    char* parentBranch[10];
    int parLen = 0;
    char *itemName = NULL;
    

    if (fields[TYPE] != NULL) {
        getElements(fields[NAME],parentBranch ,&parLen);
        itemName = parentBranch[--parLen];
        if(strcmp(fields[TYPE], "branch") == 0) {
            if(parLen == 0) {
                printf(" Branch connected to main node = %s \n" , itemName);
                struct branch * br = (struct branch *)malloc(sizeof(struct branch));
                br->branch_name = strdup(itemName);
                br->from = main_node;
                br->to = (struct node *)malloc(sizeof(struct node));
                br->to->branch_size = 0;
                br->to->signal_size = 0;
                br->description = strdup(fields[DESC]);
               // printf("Branch on Main node Name = %s  parent name = %s \n", itemName , parentBranch[parLen] );
                if(main_node->branch_size == 0) {
                    main_node->branches = (struct branch **) malloc(sizeof(struct branch *));
                } else {
                    main_node->branches = (struct branch **) realloc(main_node->branches, (main_node->branch_size + 1) * sizeof(struct branch *));
                }

                main_node->branches[main_node->branch_size] = br;
                main_node->branch_size = main_node->branch_size + 1;
           } else {
               struct node* childNode = getNode(main_node, parentBranch, parLen );
               if(childNode == NULL) {
                   printf("child node null for branch %s %s \n", itemName, parentBranch[parLen -1]);
                   return;
               }
               struct branch * br = (struct branch *)malloc(sizeof(struct branch));
               br->branch_name = strdup(itemName);
               br->from = childNode;
               br->to = (struct node *)malloc(sizeof(struct node));
               br->to->branch_size = 0;
               br->to->signal_size = 0;
               br->description = strdup(fields[DESC]);
               
               if(childNode->branch_size == 0) {
                   childNode->branches = (struct branch **) malloc(sizeof(struct branch *));
               } else {
                   childNode->branches = (struct branch **) realloc(childNode->branches, (childNode->branch_size + 1) * sizeof(struct branch *));
               }
               childNode->branches[childNode->branch_size] = br;
               childNode->branch_size = childNode->branch_size + 1;
            }
        } else {
            struct node* childNode = getNode(main_node, parentBranch, parLen );
            if(childNode == NULL) {
                printf("child node null for signal %s %s \n", itemName, parentBranch[parLen -1]);
                return;
            }
            struct signal * sig = (struct signal *)malloc(sizeof(struct signal));
            sig->signal_name = strdup(itemName);
            sig->signal_full_name = strdup(fields[NAME]);
            sig->id = atoi(fields[ID]);

            if(fields[UNIT] != NULL)
               sig->unit = strdup(fields[UNIT]);
            else
                 sig->unit =  NULL;
            if(fields[DESC] != NULL)
                sig->description = strdup(fields[DESC]);
            else
                 sig->description =  NULL;
            if(fields[TYPE] != NULL)
                 sig->value_type = strdup(fields[TYPE]);
            else
                 sig->value_type = NULL;
            if(fields[POSSIBLE_VAL] != NULL)
                sig->possible_values = strdup(fields[POSSIBLE_VAL]);
            else
                sig->possible_values = NULL;

            allocateValueSpace(sig);

            if(childNode->signal_size == 0) {
                childNode->signals = (struct signal **) malloc(sizeof(struct signal *));
            } else {
                childNode->signals = (struct signal **) realloc(childNode->signals, (childNode->signal_size + 1) * sizeof(struct signal *));
            }
            childNode->signals[childNode->signal_size] = sig;
            childNode->signal_size = childNode->signal_size + 1;

            // add the pointer to the array based on the ID.
            ptrSignals[sig->id] = sig;

        }
    }
}


/**
 * @brief Initializes VSS server. Creates VSS signal tree.
 * @param mainNode -  Main node of the tree. Needs to be passed by the calling application.
 * @param ptrSignals - Array for storing pointers to signals form the tree for easy access. The indices are the signal IDs.
 */
void initVSS (struct node* mainNode, struct signal** ptrSignals) {

    FILE* stream = fopen("vss_rel_1.0.csv", "r");
    char line[1024];
    char* fields[8];
    int i = 0;
    mainNode->branch_size = 0;
    mainNode->branches = NULL;
    while (fgets(line, 1024, stream))
    {
         getfields(line , fields);
        createTree(fields , mainNode , ptrSignals);
        i++;
    }

    fclose(stream);
}

int findStar(char *path){
    for(char i=0 ; i< getLength(path) ; i++) {
        if(path[i] == '*') {
            return i;
        }
  }
   return 0;
}

int getNodesRecursively (struct node * searchNodes[], int searchNodelen, char* parentBranches[] , int pBranchLen , int level , struct node * retNodes[]) {

      int retNodesize = 0;

      // iterate through all the nodes to be sarched
      for(int i=0 ; i< searchNodelen ; i++) {
         
           // iterate through all the branches under the node.
           for(int j=0 ; j < searchNodes[i]->branch_size ; j++) { 
                if(strcmp(parentBranches[level], "*") == 0) {
                 if( retNodesize > 0) {
                      retNodes[retNodesize++] = searchNodes[i]->branches[j]->to;
                  } else {
                      retNodes[retNodesize++] = searchNodes[i]->branches[j]->to;
                  }
                } else {
                     if(strcmp(searchNodes[i]->branches[j]->branch_name , parentBranches[level]) == 0) {
                          retNodes[retNodesize++] = searchNodes[i]->branches[j]->to;
                          return 1;
                     }
                   
                }
           }           

     }
   return retNodesize;
}

int getNodes (struct node* mainNode ,char** parentBranches , int pBranchLen , struct node ** retNodes) {

 struct node* searchNodes[MAX_TREENODES];
 int len = -1;
    for(int i=0 ; i < pBranchLen ; i++) {
        struct node * nodes[MAX_TREENODES];
        if(len == -1) {
           searchNodes[0] = mainNode;
           len = 1;
        } 
        len = getNodesRecursively(searchNodes ,len, parentBranches , pBranchLen, i,nodes);
        memcpy(searchNodes, nodes , len * sizeof(struct node *));

    }
 // memory copy required.
 memcpy(retNodes , searchNodes , (len * sizeof(struct node *)));
 return len;

}


/**
 * @brief Returns a signal from the VSS signal tree based on the path passed.
 * @param mainNode - Main node of the tree.
 * @param path - Path of the signal.
 * @param sig - Assigned memory for the found signal.
 * @return - len of signal array if successful, -1 if no signal found,-2 if the parent node not found, -3 if the path contains no signals.
 */
int getSignalsFromTree(struct node * mainNode, char* sigpath, struct signal sig[]) {

    char* parentBranches[MAX_PARENT_BRANCHES];
    char* sigName;
    
    int parLen = 0;

    struct node* parentNodes[MAX_TREENODES];

    getElements(sigpath, parentBranches ,&parLen);
    sigName = parentBranches[--parLen];
    
    if(parLen == 0) {
        printf(" The path passed cannot be processed \n");
        return -1;
    }
        
    int len = getNodes(mainNode, parentBranches, parLen, parentNodes);
    printf(" getNodes returned %d nodes \n ", len);
    int sigCount = 0;

    if(strcmp(sigName,"*") == 0) {
       for( int i=0 ; i< len ; i++) {
          for( int j=0 ; j< parentNodes[i]->signal_size ; j++) {
             sig[sigCount++] = *parentNodes[i]->signals[j];
           }
       }
     } else {
         sigCount = 0;
         for( int i=0 ; i < len ; i++) {
           for( int j=0 ; j < parentNodes[i]->signal_size ; j++) {
             if(strcmp(sigName,parentNodes[i]->signals[j]->signal_name) == 0) {
                sig[sigCount++] = *parentNodes[i]->signals[j];
             }
          }
        } 
      }

      if(sigCount > 0) {
         return sigCount;
      }
    
      struct node* matchNodes[MAX_TREENODES];
      int matchNodeSize = 0;
      if(strcmp(sigName,"*") == 0) {
      // check if the branch is available.
         for( int i=0 ; i< len ; i++) {
             for(int j=0 ; j<parentNodes[i]->branch_size ; j++) {
                 matchNodes[matchNodeSize++] = parentNodes[i]->branches[j]->to;  
             }
         }
      } else {
          for( int i=0 ; i< len ; i++) {
             for(int j=0 ; j<parentNodes[i]->branch_size ; j++) {
                 if(strcmp(sigName, parentNodes[i]->branches[j]->branch_name) == 0) 
                  matchNodes[matchNodeSize++] = parentNodes[i]->branches[j]->to;  
             }
         }

      }
        sigCount = 0;
           for( int i=0 ; i< matchNodeSize ; i++) {
              for( int j=0 ; j< matchNodes[i]->signal_size ; j++) {
                sig[sigCount++] = *matchNodes[i]->signals[j];
              }
           }
          return sigCount;
    
    return -1;
 }

int setValue(int signalID , void* value) {

  struct signal* sig = ptrToSignals[signalID];

  if(sig == NULL) {
     return -1;
  }

  char * value_type = sig->value_type;
  // Check if the signal has been subscribed. If yes handle it first and then update the tree.
   
  for(int i=0 ; i < MAX_CLIENTS; i++) {
    if( subscribeHandle[signalID][i] != 0) {
       json answer;
       answer["action"] = "subscribe";
       answer["subscriptionId"] = subscribeHandle[signalID][i];

       if(strcmp( value_type , "UInt8") == 0) {
         answer["value"]  =  *((UInt8*)value);
       } else if (strcmp( value_type , "UInt16") == 0) {
         answer["value"]  =  *((UInt16*)value);
       } else if (strcmp( value_type , "UInt32") == 0) {
         answer["value"]  =  *((UInt32*)value);
       } else if (strcmp( value_type , "Int8") == 0) {
         answer["value"]  = *((Int8*)value);
       } else if (strcmp( value_type , "Int16") == 0) {
         answer["value"]  = *((Int16*)value);
       } else if (strcmp( value_type , "Int32") == 0) {
         answer["value"]  =  *((Int32*)value);
       } else if (strcmp( value_type , "Float") == 0) {
         answer["value"]  =  *((Float*)value);
       } else if (strcmp( value_type , "Double") == 0) {
         answer["value"]  = *((Double*)value);
       } else if (strcmp( value_type , "Boolean") == 0) {
         answer["value"]  =  *((Boolean*)value);
       } else if (strcmp( value_type , "String") == 0) {
         answer["value"]  = std::string((char*)value);
    }
       answer["timestamp"] = time(NULL);
       std::stringstream ss;
       ss << pretty_print(answer);
      
       subCallBack(ss.str(), subscribeHandle[signalID][i]);
     }
  }
 

  pthread_mutex_t mutex = sig->mtx;
  pthread_mutex_lock(&mutex);
  if(strcmp( value_type , "UInt8") == 0) {
     *((UInt8*)sig->value) = *((UInt8*)value);
  }else if (strcmp( value_type , "UInt16") == 0) {
     *((UInt16*)sig->value) = *((UInt16*)value);
  }else if (strcmp( value_type , "UInt32") == 0) {
     *((UInt32*)sig->value) = *((UInt32*)value);
  }else if (strcmp( value_type , "Int8") == 0) {
     *((Int8*)sig->value) = *((Int8*)value);
  }else if (strcmp( value_type , "Int16") == 0) {
     *((Int16*)sig->value) = *((Int16*)value);
  }else if (strcmp( value_type , "Int32") == 0) {
     *((Int32*)sig->value) = *((Int32*)value);
  }else if (strcmp( value_type , "Float") == 0) {
     *((Float*)sig->value) = *((Float*)value);
  }else if (strcmp( value_type , "Double") == 0) {
     *((Double*)sig->value) = *((Double*)value);
  }else if (strcmp( value_type , "Boolean") == 0) {
     *((Boolean*)sig->value) = *((Boolean*)value);
  }else if (strcmp( value_type , "String") == 0) {
     // TODO needs to be changed.
      if(((char*)sig->value) != NULL)
          free(((char*)sig->value));
      sig->value = strdup(((char*)value));
  }else {
     printf(" The value type %s is not supported \n", value_type);
     pthread_mutex_unlock(&mutex);
     return -2;
  }
 
  pthread_mutex_unlock(&mutex);

 return 0;
}

void setSubscribeCallback(void(*sendMessageToClient)(std::string, uint32_t)){
     subCallBack = sendMessageToClient;
}
