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
#include <DataSharedMem.h>
using namespace std;

// IO Thread
void * CommLink::ioRun ( void *t ) {
   CommLink *ti;
   ti = (CommLink *)t;
   ti->ioHandler();
   pthread_exit(NULL);
}

// RX Thread
void * CommLink::rxRun ( void *t ) {
   CommLink *ti;
   ti = (CommLink *)t;
   ti->rxHandler();
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
}

// Dummy RX routine
void CommLink::rxHandler() { }

// Data routine
void CommLink::dataHandler() {
   Data    *dat;
   uint    size;
   uint    *buff;
   int     slen = sizeof(net_addr_);
   time_t  ltime;
   time_t  ctime;
   uint    xmlCount;
   uint    xmlSize;
   uint    wrSize;
   int     bzerror;

   // Store time
   time(&ltime);
   ctime        = ltime;
   dataRxCount_ = 0;
   xmlCount     = xmlReqCnt_;

   // Running
   while ( runEnable_ ) {

      // Config/status/start/stop update
      if ( xmlCount != xmlReqCnt_ ) {

         // Storing is enabled
         if ( xmlStoreEn_ ) {
            wrSize = (xmlReqEntry_.length() & 0x0FFFFFFF);
            xmlSize = ((xmlType_ << 28) & 0xF0000000) | wrSize;

            // Callback function is set
            if ( dataCb_ != NULL ) dataCb_((void *)xmlReqEntry_.c_str(), xmlSize);

            // Network is open
            if ( dataNetFd_ >= 0 ) {
               sendto(dataNetFd_,&xmlSize,4,0,(const sockaddr*)&net_addr_,slen);
               sendto(dataNetFd_,xmlReqEntry_.c_str(),wrSize,0,(const sockaddr*)&net_addr_,slen);
            }

            // Shared memory
            if ( smem_ != NULL ) 
               dataSharedWrite ((DataSharedMemory *)smem_, xmlSize, xmlReqEntry_.c_str(), wrSize );

            // Data file in compressed mode
            if ( bzEnable_ ) {
               BZ2_bzWrite(&bzerror,bzFile_,&xmlSize,4);
               BZ2_bzWrite(&bzerror,bzFile_,(void *)xmlReqEntry_.c_str(),wrSize);
            }

            // Data file is open
            else if ( dataFileFd_ >= 0 ) {
               write(dataFileFd_,&xmlSize,4);
               write(dataFileFd_,xmlReqEntry_.c_str(),wrSize);
            }
         }
         xmlCount = xmlReqCnt_;
         xmlRespCnt_++;
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

         // Shared memory
         if ( smem_ != NULL ) 
            dataSharedWrite ((DataSharedMemory *)smem_, size, (char *)buff, size*4 );

         // Data file in compressed mode
         if ( bzEnable_ ) {
            BZ2_bzWrite(&bzerror,bzFile_,&size,4);
            BZ2_bzWrite(&bzerror,bzFile_,buff,size*4);
            dataFileCount_++;
         }

         // File is open
         else if ( dataFileFd_ >= 0 ) {
            write(dataFileFd_,&size,4);
            write(dataFileFd_,buff,size*4);
            dataFileCount_++;
         }
         dataRxCount_++;
         delete dat;

         // Debug once a second
         if ( debug_ ) {
            time(&ctime);
            if ( ltime != ctime ) {
               cout << "CommLink::dataHandler -> Received data. Size = " << dec << size
                    << ", TotCount = " << dec << dataRxCount_
                    << ", FileCount = " << dec << dataFileCount_ 
                    << ", Buffer Depth = " << dec << dataQueue_.entryCnt() << endl;
               ltime = ctime;
            }
         }
      }
      else usleep(1);
   }
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
   xmlReqEntry_    = "";
   xmlType_        = 0;
   xmlReqCnt_      = 0;
   xmlRespCnt_     = 0;
   xmlStoreEn_     = true;
   toDisable_      = false;
   smem_           = NULL;
   bzEnable_       = false;

   pthread_mutex_init(&mutex_,NULL);
}

// Deconstructor
CommLink::~CommLink ( ) { 
   close();
   if ( smem_ != NULL ) dataSharedClose((DataSharedMemory*)smem_);
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

   // Start rx thread
   if ( pthread_create(&rxThread_,NULL,rxRun,this) ) {
      err << "CommLink::open -> Failed to create rxThread" << endl;
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

   // Wait for thread to stop
   pthread_join(ioThread_, NULL);
   pthread_join(rxThread_, NULL);
   pthread_join(dataThread_, NULL);
}

// Open data file
void CommLink::openDataFile (string file, bool compress) {
   stringstream tmp;
   int          bzerror;
   FILE         *f;

   dataFile_ = file;
   bzEnable_ = compress;

   // Compress mode
   if ( bzEnable_ ) {
      dataFileFd_ = -1;

      // Open file
      umask(002);
      f = fopen ( file.c_str(), "a" );

      // Attempt to compress file
      if ( f ) {
         bzFile_ = BZ2_bzWriteOpen(&bzerror,f,9,0,30);
         if ( bzerror != BZ_OK ) bzEnable_ = false;
      }
      else bzEnable_ = false;

      // Status
      tmp.str("");
      tmp << "CommLink::openDataFile -> ";
      if ( dataFileFd_ < 0 ) tmp << "Error opening compressed data file ";
      else tmp << "Opened compressed data file ";
      tmp << file << endl;

      // Debug result
      if ( debug_ ) cout << tmp.str();
      dataFileCount_ = 0;

      // Status
      if ( ! bzEnable_ ) throw(tmp.str());
   }

   // Non compress mode
   else {

      // Open the file
      dataFileFd_ = ::open(file.c_str(),O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

      // Status
      tmp.str("");
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
}

// Close data file
void CommLink::closeDataFile () {
   int  bzerror;

   // Close compression
   if ( bzEnable_ ) {
      bzEnable_ = false;
      BZ2_bzWriteClose(&bzerror,bzFile_,0,NULL,NULL);
   }
   else {
      ::close(dataFileFd_);
      dataFileFd_  = -1;
   }

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
   uint         tryCount;
   bool         error = false;

   pthread_mutex_lock(&mutex_);

   if ( (reg->size()+3) > maxRxTx_ ) {
      err.str("");
      err << "CommLink::queueRegister -> Register: " << reg->name();
      err << "Register size exceeds maxRxTx!";
      if ( debug_ ) cout << err.str() << endl;
      error = true;
   }
   else {

      // Setup request
      timer        = 0;
      regReqEntry_ = reg;
      regReqDest_  = destination;
      regReqWrite_ = write;
      currResp     = regRespCnt_;
      tryCount     = 0;

      do {
         regReqCnt_++;
         tryCount++;
         error = false;

         // Wait for response
         while ( wait && !error && currResp == regRespCnt_ ) {
            if ( timer > 1000 && (!toDisable_)) {
               err.str("");
               err << "CommLink::queueRegister -> Register: " << reg->name();
               if ( write ) err << ", Write"; else err << ", Read";
               err << ", Destination: 0x" << hex << setw(8) << setfill('0') << destination;
               err << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
               err << ", Attempt: " << dec << tryCount;
               err << ", Timeout, Trying Again!";
               cout << err.str() << endl;
               timeoutCount_++;
               timer = 0;
               error = true;
               usleep(100);
            }
            else {
               timer++;
               usleep(1);
            }
         }

         if ( !error ) regRxCount_++;

         // Check status value
         if ( wait && !error && (reg->status() != 0) ) {
            err.str("");
            err << "CommLink::queueRegister -> Register: " << reg->name();
            if ( write ) err << ", Write"; else err << ", Read";
            err << ", Destination: 0x" << hex << setw(8) << setfill('0') << destination;
            err << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
            err << ", Status: 0x" << hex << setw(8) << setfill('0') << reg->status();
            err << ", Attempt: " << dec << tryCount;
            err << ", Status Error, Trying Again!";
            cout << err.str() << endl;
            error = true;
            usleep(100);
         }
         regRxCount_++;

      } while ( error && tryCount <= 5 );

      // Error occured
      if ( error ) {
         err.str("");
         err << "CommLink::queueRegister -> Register: " << reg->name();
         if ( write ) err << ", Write"; else err << ", Read";
         err << ", Destination: 0x" << hex << setw(8) << setfill('0') << destination;
         err << ", Address: 0x" << hex << setw(8) << setfill('0') << reg->address();
         err << ", Failed!!!!";
         cout << err.str() << endl;
      }
      else reg->clrStale();
   }
   pthread_mutex_unlock(&mutex_);

   if ( error ) throw(err.str());
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
      if ( timer > 1000 && (!toDisable_)) {
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

// Queue data transmission
void CommLink::queueDataTx ( uint destination, uint address, uint *txBuffer, uint txLength ) {
   uint         timer;
   stringstream err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer          = 0;
   dataReqEntry_  = txBuffer;
   dataReqLength_ = txLength;
   dataReqDest_   = destination;
   dataReqAddr_   = address;
   dataReqCnt_++;

   //Check that exactly one lane and VC are selected
   uint lane    = (dataReqAddr_ >> 4) & 0xF;
   uint vc      = (dataReqAddr_     ) & 0xF;
   uint laneSum = 0;
   uint vcSum   = 0;
   for ( uint i = 0; i < 4; ++i ) {
      laneSum += (lane >> i) & 0x1;
      vcSum   += (vc   >> i) & 0x1;
   }

   if ( vcSum != 1 || laneSum != 1 ) {
         err.str("");
         err << "CommLink::queueDataTx -> ";
         err << " bad VC/lane selection: ";
         err << ", Lane: 0x" << hex << setw(8) << setfill('0') << lane;
         err << ", Vc: 0x" << hex << setw(8) << setfill('0') << vc;
         pthread_mutex_unlock(&mutex_);
         throw(err.str());

   }
   else if ( txLength > maxRxTx_ ) {
         err.str("");
         err << "CommLink::queueDataTx -> ";
         err << ", Destination: 0x" << hex << setw(8) << setfill('0') << destination;
         err << ", Size: " << dec << txLength;
         err << ", First word: " << hex << setw(8) << setfill('0') << txBuffer[0];
         err << ", Exceeded maxRxTx_: " << dec << maxRxTx_;
         pthread_mutex_unlock(&mutex_);
         throw(err.str());

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

}

// Add configuration to data file
void CommLink::addConfig ( string config ) {
   uint   currResp;
   uint   timer;
   string err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   xmlReqEntry_ = config;
   xmlType_     = Data::XmlConfig;
   currResp     = xmlRespCnt_;
   xmlReqCnt_++;

   // Wait for response
   while ( currResp == xmlRespCnt_ ) {
      if ( timer > 1000000 ) {
         cout << "CommLink::addConfig -> Waiting for main thread!" << endl;
         timer = 0;
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
   xmlReqEntry_ = status;
   xmlType_     = Data::XmlStatus;
   currResp     = xmlRespCnt_;
   xmlReqCnt_++;

   // Wait for response
   while ( currResp == xmlRespCnt_ ) {
      if ( timer > 1000000 ) {
         cout << "CommLink::addStatus -> Waiting for main thread!" << endl;
         timer = 0;
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}

// Add run start to data file
void CommLink::addRunStart ( string xml ) {
   uint   currResp;
   uint   timer;
   string err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   xmlReqEntry_ = xml;
   xmlType_     = Data::XmlRunStart;
   currResp     = xmlRespCnt_;
   xmlReqCnt_++;

   // Wait for response
   while ( currResp == xmlRespCnt_ ) {
      if ( timer > 1000000 ) {
         cout << "CommLink::addRunStart -> Waiting for main thread!" << endl;
         timer = 0;
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}

// Add status to data file
void CommLink::addRunStop ( string xml ) {
   uint   currResp;
   uint   timer;
   string err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   xmlReqEntry_ = xml;
   xmlType_     = Data::XmlRunStop;
   currResp     = xmlRespCnt_;
   xmlReqCnt_++;

   // Wait for response
   while ( currResp == xmlRespCnt_ ) {
      if ( timer > 1000000 ) {
         cout << "CommLink::addRunStop -> Waiting for main thread!" << endl;
         timer = 0;
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}

// Add timestamp to data file
void CommLink::addRunTime ( string xml ) {
   uint   currResp;
   uint   timer;
   string err;

   pthread_mutex_lock(&mutex_);

   // Setup request
   timer        = 0;
   xmlReqEntry_ = xml;
   xmlType_     = Data::XmlRunTime;
   currResp     = xmlRespCnt_;
   xmlReqCnt_++;

   // Wait for response
   while ( currResp == xmlRespCnt_ ) {
      if ( timer > 1000000 ) {
         cout << "CommLink::addRunTime -> Waiting for main thread!" << endl;
         timer = 0;
      }
      timer++;
      usleep(1);
   }
   pthread_mutex_unlock(&mutex_);
}


// Enable store of config/status/start/stop to data file & callback
void CommLink::setXmlStore ( bool enable ) {
   xmlStoreEn_ = enable;
}

// Enable shared 
void CommLink::enableSharedMemory ( string system, uint id ) {

   // Attempt to open and init shared memory
   if ( (smemFd_ = dataSharedOpenAndMap ( (DataSharedMemory **)(&smem_) , system.c_str(), id )) < 0 ) {
      smem_ = NULL;
      throw string("CommLink::enabledSharedMemory -> Failed to open shared memory");
   }

   // Init shared memory
   dataSharedInit((DataSharedMemory*)smem_);
}

