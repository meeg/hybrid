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
using namespace std;

// Constructor
DataRead::DataRead ( ) {
   fd_          = -1;
   size_        = 0;
   sawRunStart_ = false;
   sawRunStop_  = false;
}

// Deconstructor
DataRead::~DataRead ( ) { }

// Process xml
void DataRead::xmlParse ( uint size ) {
   char         *buff;
   uint         mySize;
   uint         myType;

   // Decode size
   myType = (size >> 28) & 0xF;
   mySize = (size & 0x0FFFFFFF);

   //cout << "Found Marker: Type=" << dec << myType << ", Size=" << dec << mySize << endl;

   // Read file
   buff = (char *) malloc(mySize+1);
   if ( ::read(fd_, buff, mySize) != (int)mySize) {
      cout << "DataRead::xmlParse -> Read error!" << endl;
      return;
   }
   buff[mySize-1] = 0;

   if ( myType == Data::XmlConfig   ) config_.parse("config",buff);
   if ( myType == Data::XmlStatus   ) status_.parse("status",buff);
   if ( myType == Data::XmlRunStart ) {
      //cout << "-----------XML Start---------------" << endl;
      //cout << buff << endl;
      //cout << "-----------------------------------" << endl;
      start_.parse("runStart",buff);
   }
   if ( myType == Data::XmlRunStop  ) {
      //cout << "-----------XML Stop----------------" << endl;
      //cout << buff << endl;
      //cout << "-----------------------------------" << endl;
      stop_.parse("runStop",buff);
   }

   free(buff);
}

// Open file
bool DataRead::open ( string file ) {
   size_ = 0;
   status_.clear();
   config_.clear();

   // Attempt to open file
   if ( (fd_ = ::open (file.c_str(),O_RDONLY | O_LARGEFILE)) < 0 ) {
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

//! Return file size in bytes
off_t DataRead::size ( ) {
   off_t curr;

   if ( fd_ < 0 ) return(0);
   if ( size_ == 0 ) {
      curr  = lseek(fd_, 0, SEEK_CUR);
      size_ = lseek(fd_, 0, SEEK_END);
      lseek(fd_, curr, SEEK_SET);
   }
   return(size_);
}

//! Return file position in bytes
off_t DataRead::pos ( ) {
   if ( fd_ < 0 ) return(0);
   return(lseek(fd_, 0, SEEK_CUR));
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

         // Start
         case Data::XmlRunStart : sawRunStart_ = true; xmlParse(size); break;

         // Stop
         case Data::XmlRunStop : sawRunStop_ = true; xmlParse(size); break;

         // Unknown
         default: 
            cout << "DataRead::next -> Unknown data type 0x" 
                 << hex << setw(8) << setfill('0') << ((size >> 28) & 0xF) << " skipping." << endl;
            return(lseek(fd_, (size & 0x0FFFFFFF), SEEK_CUR));
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
   return(config_.get(var));
}

// Get a status value
string DataRead::getStatus ( string var ) {
   return(status_.get(var));
}

// Get a config value
uint DataRead::getConfigInt ( string var ) {
   return(config_.getInt(var));
}

// Get a status value
uint DataRead::getStatusInt ( string var ) {
   return(status_.getInt(var));
}

// Dump config
void DataRead::dumpConfig ( ostream &out ) {
   out << "Dumping current config variables:" << endl;
   out << config_.getList("   Config: ");
}

// Dump status
void DataRead::dumpStatus ( ) {
   cout << "Dumping current status variables:" << endl;
   cout << status_.getList("   Status: ");
}

//! Get config as XML
string DataRead::getConfigXml ( ) {
   string ret;
   ret = "";
   ret.append("<system>\n");
   ret.append("   <config>\n");
   ret.append(config_.getXml());
   ret.append("   </config>\n");
   ret.append("</system>\n");
   return(ret);
}

//! Dump status
string DataRead::getStatusXml ( ) {
   string ret;
   ret = "";
   ret.append("<system>\n");
   ret.append("   <status>\n");
   ret.append(status_.getXml());
   ret.append("   </status>\n");
   ret.append("</system>\n");
   return(ret);
}

// Get a start value
string DataRead::getRunStart ( string var ) {
   return(start_.get(var));
}

// Get a stop value
string DataRead::getRunStop ( string var ) {
   return(stop_.get(var));
}

// Dump start
void DataRead::dumpRunStart ( ) {
   cout << "Dumping run start variables:" << endl;
   cout << start_.getList("   RunStart: ");
}

// Dump stop
void DataRead::dumpRunStop ( ) {
   cout << "Dumping run stop variables:" << endl;
   cout << stop_.getList("    RunStop: ");
}

// Return true if we saw start marker, self clearing
bool  DataRead::sawRunStart ( ) {
   bool ret = sawRunStart_;
   sawRunStart_ = false;
   return(ret);
}

// Return true if we saw stop marker, self clearing
bool  DataRead::sawRunStop ( ) {
   bool ret = sawRunStop_;
   sawRunStop_ = false;
   return(ret);
}

