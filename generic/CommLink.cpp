//-----------------------------------------------------------------------------
// File          : CommLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic communications link
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <CommLink.h>
#include <Register.h>
#include <Command.h>
#include <Data.h>
#include <CommQueue.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
using namespace std;

// IO Thread
void * CommLink::ioRun ( void *t ) {
   CommLink *ti;
   ti = (CommLink *)t;
   ti->ioHandler();
   pthread_exit(NULL);
}

// Data Thread
void * CommLink::dataRun ( void *t ) {
   CommLink *ti;
   ti = (CommLink *)t;
   ti->dataHandler();
   pthread_exit(NULL);
}

// Dummy IO routine
void CommLink::ioHandler() {
   uint      lastReqCnt;
   uint      lastCmdCnt;
   uint      lastRunCnt;

   lastReqCnt = regReqCnt_;
   lastCmdCnt = cmdReqCnt_;
   lastRunCnt = runReqCnt_;

   while ( runEnable_ ) {
      if ( lastReqCnt != regReqCnt_ ) {
         lastReqCnt = regReqCnt_;
         regRespCnt_++;
      }
      if ( lastCmdCnt != cmdReqCnt_ ) {
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }
      if ( lastRunCnt != runReqCnt_ ) lastRunCnt = runReqCnt_;
      usleep(1);
   }
   pthread_exit(NULL);
}

// Data routine
void CommLink::dataHandler() {
   Data    *dat;
   uint    size;
   uint    *buff;
   int     slen = sizeof(net_addr_);
   time_t  ltime;
   time_t  ctime;
   uint    cfgCount;
   uint    cfgSize;
   uint    wrSize;

   // Store time
   time(&ltime);
   ctime        = ltime;
   dataRxCount_ = 0;
   cfgCount     = cfgReqCnt_;

   // Running
   while ( runEnable_ ) {

      // Config/status update
      if ( cfgCount != cfgReqCnt_ ) {
         wrSize = (cfgReqEntry_.length() & 0x0FFFFFFF);
         if ( cfgIsConfig_ ) cfgSize = wrSize | 0x10000000;
         else cfgSize = wrSize | 0x20000000;

         // Callback function is set
         if ( dataCb_ != NULL ) dataCb_((void *)cfgReqEntry_.c_str(), cfgSize);

         // Network is open
         if ( dataNetFd_ >= 0 ) {
            sendto(dataNetFd_,&cfgSize,4,0,(const sockaddr*)&net_addr_,slen);
            sendto(dataNetFd_,cfgReqEntry_.c_str(),wrSize,0,(const sockaddr*)&net_addr_,slen);
         }

         // Data file is open
         if ( dataFileFd_ >= 0 ) {
            write(dataFileFd_,&cfgSize,4);
            write(dataFileFd_,cfgReqEntry_.c_str(),wrSize);
         }
         cfgCount = cfgReqCnt_;
         cfgRespCnt_++;
      }

      // Data is ready
      if ( dataQueue_.ready() ) {

         // Get entry
         dat = (Data *)dataQueue_.pop();
         size = dat->size();
         buff = dat->data();

         // Callback function is set
         if ( dataCb_ != NULL ) dataCb_(buff, size);

         // Network is open
         if ( dataNetFd_ >= 0 ) {
            sendto(dataNetFd_,&size,4,0,(const sockaddr*)&net_addr_,slen);
            sendto(dataNetFd_,buff,size*4,0,(const sockaddr*)&net_addr_,slen);
         }

         // File is open
         if ( dataFileFd_ >= 0 ) {
            write(dataFileFd_,&size,4);
            write(dataFileFd_,buff,size*4);
            dataFileCount_++;
         }
         dataRxCount_++;

         // Debug once a second
         if ( debug_ ) {
            time(&ctime);
            if ( ltime != ctime ) {
               cout << "CommLink::dataHandler -> Received data. Size = " << dec << size
                    << ", TotCount = " << dec << dataRxCount_
                    << ", FileCount = " << dec << dataFileCount_ << endl;
               ltime = ctime;
            }
         }
         delete dat;
      }
      else usleep(1);
   }
   pthread_exit(NULL);
}

// Constructor
CommLink::CommLink ( ) {
   debug_          = false;
   dataFileFd_     = -1;
   dataFile_       = "";
   dataNetFd_      = -1;
   dataNetAddress_ = "";
   dataNetPort_    = 0;
   regReqEntry_    = NULL;
   regReqDest_     = 0;
   regReqCnt_      = 0;
   regReqWrite_    = false;
   regRespCnt_     = 0;
   cmdReqEntry_    = NULL;
   cmdReqDest_     = 0;
   cmdReqCnt_      = 0;
   cmdRespCnt_     = 0;
   runReqEntry_    = NULL;
   runReqDest_     = 0;
   runReqCnt_      = 0;
   runEnable_      = false;
   dataFileCount_  = 0;
   dataRxCount_    = 0;
   regRxCount_     = 0;
   timeoutCount_   = 0;
   errorCount_     = 0;
   maxRxTx_        = 4;
   dataCb_         = NULL;
   unexpCount_     = 0;
   cfgReqEntry_    = "";
   cfgIsConfig_    = false;
   cfgReqCnt_      = 0;
   cfgRespCnt_     = 0;

   rxBuff_ = (uint *)malloc(maxRxTx_*sizeof(uint));
   txBuff_ = (uint *)malloc(maxRxTx_*sizeof(uint));

   pthread_mutex_init(&mutex_,NULL);
}

// Deconstructor
CommLink::~CommLink ( ) { 
   close();
}

// Open link and start threads
void CommLink::open () {
   stringstream err;
   runEnable_ = true;

   err.str("");

   // Start io thread
   if ( pthread_create(&ioThread_,NULL,ioRun,this) ) {
      err << "CommLink::open -> Failed to create ioThread" << endl;
      if ( debug_ ) cout << err.str();
      close();
      throw(err.str());
   }

   // Start data thread
   if ( pthread_create(&dataThread_,NULL,dataRun,this) ) {
      err << "CommLink::open -> Failed to create dataThread" << endl;
      if ( debug_ ) cout << err.str();
      close();
      throw(err.str());
   }

   // Let threads catch up
   usleep(100);
}

// Stop threads and close link
void CommLink::close () {

   // Stop the thread
   runEnable_ = false;

   usleep(100);

   // Wait for thread to stop
   //pthread_join(ioThread_, NULL);

   // Wait for thread to stop
   //pthread_join(dataThread_, NULL);
}

// Open data file
void CommLink::openDataFile (string file) {
   stringstream tmp;
   dataFile_ = file;

   // Attempt to open data file
   dataFileFd_ = ::open(file.c_str(),O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
   tmp.str("");

   // Status
   tmp << "CommLink::openDataFile -> ";
   if ( dataFileFd_ < 0 ) tmp << "Error opening data file ";
   else tmp << "Opened data file ";
   tmp << file << endl;

   // Debug result
   if ( debug_ ) cout << tmp.str();
   dataFileCount_ = 0;

   // Status
   if ( dataFileFd_ < 0 ) throw(tmp.str());
}

// Close data file
void CommLink::closeDataFile () {
   ::close(dataFileFd_);
   dataFileFd_  = -1;

   if ( debug_ ) {
      cout << "CommLink::closeDataFile -> "
           << "Closed data file " << dataFile_
           << ", Count = " << dec << dataFileCount_ << endl;
   }
   dataFileCount_ = 0;
}

// Open data network
void CommLink::openDataNet (string address, int port) {
   stringstream dbg;

   dataNetAddress_ = address;
   dataNetPort_    = port;
   dbg.str("");

   // Create socket
   dataNetFd_ = socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP); 
   if ( dataNetFd_ == -1 ) {
      dbg << "CommLink::openDataNet -> ";
      dbg << "Error createing UDP socket" << endl;
      if ( debug_ ) cout << dbg.str();
      throw(dbg.str());
   }

   // Setup address
   memset((char *)&net_addr_, 0, sizeof(net_addr_));
   net_addr_.sin_family = AF_INET;
   net_addr_.sin_port = htons(port);
   if ( inet_aton(address.c_str(),&net_addr_.sin_addr) == 0 ) {
      dbg << "CommLink::openDataNet -> ";
      dbg << "Error resolving UDP address: " << address << endl;
      if ( debug_ ) cout << dbg.str();
      dataNetFd_ = -1;
      throw(dbg.str());
   }

   // Debug result
   if ( debug_ ) {
      cout << "CommLink::openDataNet -> Opened network connection " << address << endl;
   }
}

// Close data file
void CommLink::closeDataNet () {
   ::close(dataNetFd_);
   dataNetFd_  = -1;

   if ( debug_ ) {
      cout << "CommLink::closeData -> "
           << "Closed data network " << dataNetAddress_ << endl;
   }
}

//! Set data callback function
void CommLink::setDataCb ( void (*dataCb)(void *, uint)) {
   dataCb_ = dataCb;
}

// Set debug flag
void CommLink::setDebug( bool enable ) {
   debug_ = enable;
}

// Queue register request
void CommLink::queueRegister ( uint destination, Register *reg, bool write, bool wait ) {
   uint         currResp;
   uint         timer;
   stringstream err;

   if ( (reg->size()+3) > maxRxTx_ ) {
      err.str("");
      err << "CommLink::queueRegister -> Register: " << reg->name();
      err << "Register size exceeds maxRxTx!";
      if ( debug_ ) cout << err.str() << endl;
      throw(err.str());
   }

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   regReqEntry_ = reg;
   regReqDest_  = destination;
   regReqWrite_ = write;
   currResp     = regRespCnt_;
   regReqCnt_++;

   // Wait for response
   while ( wait && currResp == regRespCnt_ ) {
      if ( timer > 1000 ) {
         err.str("");
         err << "CommLink::queueRegister -> Register: " << reg->name();
         err << ", Write: " << dec << write;
         err << ", Destination: 0x" << hex << setw(8) << setfill('0') << destination;
         err << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
         err << ", Timeout!";
         if ( debug_ ) cout << err.str() << endl;
         timeoutCount_++;
         reg->set(0xFFFFFFFF);
         memset(reg->data(),0xFF,(reg->size()*4));
         pthread_mutex_unlock(&mutex_);
         throw(err.str());
      }
      timer++;
      usleep(1);
   }
   regRxCount_++;

   // Clear stale flag
   reg->clrStale();

   pthread_mutex_unlock(&mutex_);
}

// Queue command request
void CommLink::queueCommand ( uint destination, Command *cmd ) {
   uint         currResp;
   uint         timer;
   stringstream err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   cmdReqEntry_ = cmd;
   cmdReqDest_  = destination;
   currResp     = cmdRespCnt_;
   cmdReqCnt_++;

   // Wait for response
   while ( currResp == cmdRespCnt_ ) {
      if ( timer > 1000 ) {
         err.str("");
         err << "CommLink::queueCommand -> Command: " << cmd->name();
         err << ", Destination: 0x" << hex << setw(8) << setfill('0') << destination;
         err << ", OpCode: 0x" << hex << setw(4) << setfill('0') << cmd->opCode();
         err << ", Timeout!";
         if ( debug_ ) cout << err.str() << endl;
         timeoutCount_++;
         pthread_mutex_unlock(&mutex_);
         throw(err.str());
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}

// Queue run command request
void CommLink::queueRunCommand ( ) {
   pthread_mutex_lock(&mutex_);
   if ( runReqEntry_ != NULL ) runReqCnt_++;
   pthread_mutex_unlock(&mutex_);
}

// Set run command request
void CommLink::setRunCommand ( uint destination, Command *cmd ) {
   pthread_mutex_lock(&mutex_);
   runReqEntry_ = cmd;
   runReqDest_  = destination;
   pthread_mutex_unlock(&mutex_);
}

// Get data count
uint CommLink::dataFileCount () {
   return(dataFileCount_);
}

// Get data receive count
uint CommLink::dataRxCount() {
   return(dataRxCount_);
}

// Get register rx count
uint CommLink::regRxCount() {
   return(regRxCount_);
}

// Get timeout count
uint CommLink::timeoutCount() {
   return(timeoutCount_);
}

// Get error count
uint CommLink::errorCount() {
   return(errorCount_);
}

// Get unexpcted count
uint CommLink::unexpectedCount() {
   return(unexpCount_);
}

// Clear counters
void CommLink::clearCounters() {
   dataFileCount_ = 0;
   dataRxCount_   = 0;
   regRxCount_    = 0;
   timeoutCount_  = 0;
   errorCount_    = 0;
   unexpCount_    = 0;
}


// Set mask for data reception
void CommLink::setDataMask ( uint mask ) {
   dataMask_ = mask;
}

// Set max rx size
void CommLink::setMaxRxTx (uint size) {
   stringstream err;

   // Start io thread
   if ( runEnable_ ) {
      err << "CommLink::setMaxRxTx -> Cannot set maxRxTx while open" << endl;
      if ( debug_ ) cout << err.str();
      throw(err.str());
   }

   maxRxTx_ = size;

   free(rxBuff_);
   free(txBuff_);

   rxBuff_ = (uint *)malloc(maxRxTx_*sizeof(uint));
   txBuff_ = (uint *)malloc(maxRxTx_*sizeof(uint));
}

// Add configuration to data file
void CommLink::addConfig ( string config ) {
   uint   currResp;
   uint   timer;
   string err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   cfgReqEntry_ = config;
   cfgIsConfig_ = true;
   currResp     = cfgRespCnt_;
   cfgReqCnt_++;

   // Wait for response
   while ( currResp == cfgRespCnt_ ) {
      if ( timer > 1000 ) {
         err = "CommLink::addConfig -> Timeout!";
         if ( debug_ ) cout << err << endl;
         pthread_mutex_unlock(&mutex_);
         throw(err);
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}

// Add status to data file
void CommLink::addStatus ( string status ) {
   uint   currResp;
   uint   timer;
   string err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   cfgReqEntry_ = status;
   cfgIsConfig_ = false;
   currResp     = cfgRespCnt_;
   cfgReqCnt_++;

   // Wait for response
   while ( currResp == cfgRespCnt_ ) {
      if ( timer > 1000 ) {
         err = "CommLink::addConfig -> Timeout!";
         if ( debug_ ) cout << err << endl;
         pthread_mutex_unlock(&mutex_);
         throw(err);
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}

