//-----------------------------------------------------------------------------
// File          : CodaServer.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/29/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Control server class
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/29/2011: created
//-----------------------------------------------------------------------------
#include <System.h>
#include <CodaServer.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
using namespace std;

// Global Variable For Callback
CodaServer *codaObject_;

// Constructor
CodaServer::CodaServer ( ) {
   debug_      = false;
   servFd_     = -1;
   connFd_     = -1;
   port_       = 0;
   system_     = NULL;
   codaObject_ = this;

   // Init buffers
   rxData_.str("");
   bzero((char *) &servAddr_,sizeof(servAddr_));
   bzero((char *) &connAddr_,sizeof(connAddr_));

   // Open shared memory
   //smemFd_ = shm_open(TRACKER_MEM_FILE, (O_CREAT | O_EXCL | O_RDWR), (S_IREAD | S_IWRITE));
   smemFd_ = shm_open(TRACKER_MEM_FILE, (O_CREAT | O_RDWR), (S_IREAD | S_IWRITE));

   // Failed to open shred memory
   if ( smemFd_ < 0 ) {
      smemFd_ = -1;
      cout << "CodaServer::CodaServer -> Failed to create shared memory" << endl;
      throw("CodaServer::CodaServer -> Failed to create shared memory");
   }
   
  // Set the size of the shared memory segment
  ftruncate(smemFd_, sizeof(TrackerSharedMemory));

  // Map the shared memory
  if((smem_ = (TrackerSharedMemory *)mmap(0, sizeof(TrackerSharedMemory), 
                      (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd_, 0)) == MAP_FAILED) {
      cout << "CodaServer::CodaServer -> Failed to map shared memory" << endl;
      throw("CodaServer::CodaServer -> Failed to map shared memory");
  }

  // Init the shared memory fields
  smem_->cmdAckCount    = 0;
  smem_->cmdRdyCount    = 0;
  smem_->dataRdyCount   = 0;
  smem_->dataAckCount   = 0;
  smem_->dataEnableMask = 0;
  smem_->dataEnableMask = 0;

  // Local init
  cmdRdyLast_ = 0;
  for (uint x=0; x < TRACKER_DATA_CHAN; x++ ) dataReady_[x] = false;
  cout << "CodaServer::CodaServer -> Shared memory is ready!" << endl;
}

// DeConstructor
CodaServer::~CodaServer ( ) {
   stopListen();
}

// Set debug flag
void CodaServer::setDebug ( bool debug ) { 
   debug_ = debug;
}

// Set top level device
void CodaServer::setSystem ( System *system ) {
   system_    = system;
}

// Start tcpip listen socket
void CodaServer::startListen ( int port ) {
   stringstream err;

   // Init structures
   servFd_ = socket(AF_INET, SOCK_STREAM, 0);
   bzero((char *)&servAddr_,sizeof(servAddr_));
   servAddr_.sin_family = AF_INET;
   servAddr_.sin_addr.s_addr = INADDR_ANY;
   servAddr_.sin_port = htons(port);

   // Attempt to bind socket
   if ( bind(servFd_, (struct sockaddr *) &servAddr_, sizeof(servAddr_)) < 0 ) {
      err.str("");
      err << "CodaServer::startListen -> Failed to bind socket " << dec << port << endl;
      servFd_ = -1;
      if ( debug_ ) cout << err.str();
      throw(err.str());
   }

   // Start listen
   listen(servFd_,5);

   // Debug
   if ( debug_ ) 
      cout << "CodaServer::startListen -> Listening on port " << dec << port << endl;
}

// Close tcpip listen socket
void CodaServer::stopListen ( ) {

   if ( connFd_ >= 0 ) close(connFd_);
   if ( servFd_ >= 0 ) close(servFd_);
   connFd_ = -1;
   servFd_ = -1;

   // Debug
   if ( servFd_ >= 0 && debug_ ) 
      cout << "CodaServer::startListen -> Stopped listening" << endl;
}

// Receive and process data if ready
void CodaServer::receive ( uint timeout ) {
   fd_set         fdset;
   int            maxFd;
   struct timeval tval;
   socklen_t      cliLen;
   int            newFd;
   char           buffer[9001];
   int            ret;
   stringstream   msg;
   int            x;
   string         pmsg;

   // Setup for listen call
   maxFd = 0;
   if ( servFd_ >= 0 ) {
      FD_SET(servFd_,&fdset);
      if ( servFd_ > maxFd ) maxFd = servFd_;
   }
   if ( connFd_ >= 0 ) {
      FD_SET(connFd_,&fdset);
      if ( connFd_ > maxFd ) maxFd = connFd_;
   }
   else rxData_.str("");

   // Call select
   tval.tv_sec  = 0;
   tval.tv_usec = timeout;
   ret = select(maxFd+1,&fdset,NULL,NULL,&tval);

   // server socket is ready
   if ( ret > 0 && servFd_ >= 0 && FD_ISSET(servFd_,&fdset)  ) {

      // Accept new client
      cliLen = sizeof(connAddr_);
      newFd = accept(servFd_,(struct sockaddr *)&connAddr_,&cliLen);

      // Error on accept
      if ( newFd < 0 ) {
         if ( debug_ ) 
            cout << "CodaServer::receive -> Failed to accept on socket" << endl;
      } else {

         // Client already connected
         if ( connFd_ >= 0 ) {
            if ( debug_ ) cout << "CodaServer::receive -> Rejected connection" << endl;
            close(newFd);
         }
         else {
            if ( debug_ ) cout << "CodaServer::receive -> Accepted new connection" << endl;
            connFd_ = newFd;

            msg.str("");
            msg << system_->structureString(false);
            msg << "\f";
            write(connFd_,msg.str().c_str(),msg.str().length());
         }
      }
   }

   // client socket is ready
   if ( ret > 0 && connFd_ >= 0 && FD_ISSET(connFd_,&fdset) ) {

      // Read data
      ret = read(connFd_,buffer,9000);

      // Connection is lost
      if ( ret <= 0 ) {
         if ( debug_ ) cout << "CodaServer::receive -> Closing connection" << endl;

         // Reset
         rxData_.str("");

         // Close socket
         close(connFd_);
         connFd_ = -1;
      }

      // Process each byte
      for (x=0; x<ret; x++) {
         if ( buffer[x] == '\f' || buffer[x] == 0x4 ) {

            // Send to top level
            if ( system_ != NULL ) {
               if ( debug_ ) {
                  cout << "CodaServer::receive -> Processing message: " << endl;
                  cout << rxData_.str() << endl;
               }
               system_->parseXmlString (rxData_.str()); 
               if ( debug_ ) cout << "CodaServer::receive -> Done Processing message" << endl;
            }
            rxData_.str("");
         }
         else rxData_ << buffer[x];
      }
   }

   // Shared memory commands
   if ( smem_->cmdRdyCount != cmdRdyLast_ ) {
      if ( debug_ ) cout << "CodaServer::receive -> Processing shared memory message: " << endl;
      cmdRdyLast_ = smem_->cmdRdyCount;
      system_->parseXmlString (smem_->cmdBuffer);
      smem_->cmdAckCount++;
      if ( debug_ ) cout << "CodaServer::receive -> Done Processing shared memory message: " << endl;
   }

   // Poll
   pmsg = system_->poll();

   // Send message
   if ( pmsg != "" ) {
      msg.str("");
      msg << pmsg;
      msg << "\f";
      write(connFd_,msg.str().c_str(),msg.str().length());
   }
}

//! Real data callback function
void CodaServer::dataCb (void *data, uint size) {
   //uint lastAck = smem_->dataAckCount;

   // Skip config data
   if ( (size & 0xF0000000) != 0 ) return;

   if ( debug_ ) cout << "CodaServer::dataCb -> Adding data to shared memory. Size=" << dec << size << endl;

   // Always FPGA 0 for now
   smem_->dataEnableMask = 0x1;
   smem_->dataSize[0]    = size;
   memcpy(smem_->dataBuffer[0],data,size*sizeof(uint));
   smem_->dataRdyCount++;

   // Wait for ack?

   if ( debug_ ) cout << "CodaServer::dataCb -> Done adding data to shared memory: " << endl;
}

//! Static data callback function
void CodaServer::staticCb (void *data, uint size) {
   codaObject_->dataCb(data,size);
}

//! Setup comm link
void CodaServer::setupCommLink ( CommLink *link ) {
   link->setDataCb(&staticCb);
}
 
