//-----------------------------------------------------------------------------
// File          : DataRead.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Read data & configuration from disk
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <DataRead.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <libxml/tree.h>
using namespace std;

// Constructor
DataRead::DataRead ( ) {
   fd_ = -1;
}

// Deconstructor
DataRead::~DataRead ( ) { }

// Remove whitespace and newlines
string DataRead::removeWhite ( string str ) {
   string temp;
   uint   i;

   temp = "";

   for (i=0; i < str.length(); i++) {
      if ( str[i] != ' ' && str[i] != '\n' ) 
         temp += str[i];
   }
   return(temp);
}

// Process xml
void DataRead::xmlParse ( uint size ) {
   xmlDocPtr    doc;
   xmlNodePtr   node;
   char         *buff;
   uint         mySize;
   uint         myType;
   string       name;

   // Decode size
   myType = (size >> 28) & 0xF;
   mySize = (size & 0x0FFFFFFF);

   // Read file
   buff = (char *) malloc(mySize);
   if ( ::read(fd_, buff, mySize) != (int)mySize) {
      cout << "DataRead::xmlParse -> Read error!" << endl;
      return;
   }
  
   // Parse string
   doc = xmlReadMemory(buff, mySize, "string.xml", NULL, 0);
   if (doc == NULL) return;

   // get the root element node
   node = xmlDocGetRootElement(doc);
   name = (char *)node->name;

   // Process based upon type
   if ( name == "config" && myType == Data::XmlConfig ) xmlLevel(node,"",true);
   else if ( name == "status" && myType == Data::XmlStatus ) xmlLevel(node,"",false);
   else cout << "Error. Config type mismatch. Name=" << name << ", Type=" << dec << myType << endl;

   // Cleanup
   xmlFreeDoc(doc);
   xmlCleanupParser();
   xmlMemoryDump();
   free(buff);
}

// Process level
void DataRead::xmlLevel( xmlNode *node, string curr, bool config ) {
   xmlNode    *childNode;
   const char *nodeName;
   char       *nodeValue;
   string     nameStr;
   string     valStr;
   char       *attrValue;
   string     idxStr;

   // Look for child nodes
   for ( childNode = node->children; childNode; childNode = childNode->next ) {

      // Copy incoming string
      nameStr = curr;

      if ( childNode->type == XML_ELEMENT_NODE ) {

         // Extract name
         nodeName  = (const char *)childNode->name;

         // Append name
         if ( nameStr != "" ) nameStr.append(":");
         nameStr.append(nodeName);

         // Append index
         attrValue = (char *)xmlGetProp(childNode,(const xmlChar*)"index");
         if ( attrValue != NULL ) {
            idxStr = attrValue;
            nameStr.append("(");
            nameStr.append(idxStr);
            nameStr.append(")");
         }

         // Process children
         xmlLevel(childNode,nameStr,config);
      }
      else if ( childNode->type == XML_TEXT_NODE ) {
         nodeValue = (char *)childNode->content;
         if ( nodeValue != NULL ) {
            valStr = nodeValue;
            if ( removeWhite(valStr) != "" ) {
               if ( config ) config_[nameStr] = valStr;
               else status_[nameStr] = valStr;
            }
         }
      }
   }
}

// Open file
bool DataRead::open ( string file ) {

	int flags = O_RDONLY;
#ifdef O_LARGEFILE
	flags |= O_LARGEFILE;
#endif
   // Attempt to open file
   if ( (fd_ = ::open (file.c_str(),flags)) < 0 ) {
      cout << "DataRead::open -> Failed to open file: " << file << endl;
      return(false);
   }
   return(true);
}

// Open file
void DataRead::close () {
   ::close(fd_);
   fd_ = -1;
}

// Get next data record
bool DataRead::next (Data *data) {
   uint size;
   bool found = false;

   if ( fd_ < 0 ) return(false);

   // Read until we get data
   do { 

      // First read frame size from data file
      if ( read(fd_,&size,4) != 4 ) return(false);

      if ( size == 0 ) continue;

      // Frame type
      switch ( (size >> 28) & 0xF ) {
         
         // Data
         case Data::RawData : found = true; break;

         // Configuration
         case Data::XmlConfig : xmlParse(size); break;

         // Status
         case Data::XmlStatus : xmlParse(size); break;

         // Unknown
         default: 
            cout << "DataRead::next -> Bad data type 0x" << hex << setw(8) << setfill('0') << size << endl;
            break;
      }
   } while ( ! found );

   // Read data      
   return(data->read(fd_,size));
}

// Get next data record
Data *DataRead::next ( ) {
   Data *tmp = new Data;
   if ( next(tmp) ) return(tmp);
   else {
      delete tmp;
      return(NULL);
   }
}

// Get a config value
string DataRead::getConfig ( string var ) {
   VariableHolder::iterator varMapIter;

   // Look for variable
   varMapIter = config_.find(var);

   // Variable was not found
   if ( varMapIter == config_.end() ) return("");
   else return(varMapIter->second);
}

// Get a status value
string DataRead::getStatus ( string var ) {
   VariableHolder::iterator varMapIter;

   // Look for variable
   varMapIter = status_.find(var);

   // Variable was not found
   if ( varMapIter == status_.end() ) return("");
   else return(varMapIter->second);
}

// Dump config
void DataRead::dumpConfig ( ostream &out ) {
   VariableHolder::iterator varMapIter;

   out << "Dumping current config variables:" << endl;
   for ( varMapIter = config_.begin(); varMapIter != config_.end(); varMapIter++ ) {
      out << "   Config: " << varMapIter->first << " = " << varMapIter->second << endl;
   }
}

// Dump status
void DataRead::dumpStatus ( ) {
   VariableHolder::iterator varMapIter;

   cout << "Dumping current status variables:" << endl;
   for ( varMapIter = status_.begin(); varMapIter != status_.end(); varMapIter++ ) {
      cout << "   Status: " << varMapIter->first << " = " << varMapIter->second << endl;
   }
}

