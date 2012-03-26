//-----------------------------------------------------------------------------
// File          : ControlServer.cpp
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
#include <ControlServer.h>
using namespace std;

// Constructor
ControlServer::ControlServer ( ) {
   debug_      = false;
   servFd_     = -1;
   port_       = 0;
   system_     = NULL;

   for ( uint x=0; x < MaxClients_; x++ ) {
      connFd_[x] = -1;
      rxData_[x].str("");
   }
   bzero((char *) &servAddr_,sizeof(servAddr_));
   bzero((char *) &connAddr_,sizeof(connAddr_));

   // Attempt to open and init shared memory
   if ( (smemFd_ = controlCmdOpenAndMap ( &smem_ )) < 0 ) 
      throw string("ControlServer::ControlServer -> Failed to open shared memory");

   // Init shared memory
   controlCmdInit(smem_);
}

// DeConstructor
ControlServer::~ControlServer ( ) {
   stopListen();
}

// Set debug flag
void ControlServer::setDebug ( bool debug ) { 
   debug_ = debug;
}

// Set top level device
void ControlServer::setSystem ( System *system ) {
   system_    = system;
}

// Start tcpip listen socket
void ControlServer::startListen ( int port ) {
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
      err << "ControlServer::startListen -> Failed to bind socket " << dec << port << endl;
      servFd_ = -1;
      if ( debug_ ) cout << err.str();
      throw(err.str());
   }

   // Start listen
   listen(servFd_,5);

   // Debug
   if ( debug_ ) 
      cout << "ControlServer::startListen -> Listening on port " << dec << port << endl;
}

// Close tcpip listen socket
void ControlServer::stopListen ( ) {
   uint x;

   for ( x=0; x < MaxClients_; x++ ) {
      if ( connFd_[x] >= 0 ) close(connFd_[x]);
      connFd_[x] = -1;
   }
   if ( servFd_ >= 0 ) close(servFd_);
   servFd_ = -1;

   // Debug
   if ( servFd_ >= 0 && debug_ ) 
      cout << "ControlServer::startListen -> Stopped listening" << endl;
}

// Receive and process data if ready
void ControlServer::receive ( uint timeout ) {
   fd_set         fdset;
   int            maxFd;
   struct timeval tval;
   socklen_t      cliLen;
   int            newFd;
   char           buffer[9001];
   int            ret;
   stringstream   msg;
   uint           x;
   int            y;
   string         pmsg;
   bool           cmdPend;

   // Setup for listen call
   FD_ZERO(&fdset);
   maxFd = 0;
   if ( servFd_ >= 0 ) {
      FD_SET(servFd_,&fdset);
      if ( servFd_ > maxFd ) maxFd = servFd_;
   }
   for ( x=0; x < MaxClients_; x++ ) {
      if ( connFd_[x] >= 0 ) {
         FD_SET(connFd_[x],&fdset);
         if ( connFd_[x] > maxFd ) maxFd = connFd_[x];
      }
      else rxData_[x].str("");
   }

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
            cout << "ControlServer::receive -> Failed to accept on socket" << endl;
      } else {

         // Find empty client
         for ( x=0; x < MaxClients_; x++ ) {
            if ( connFd_[x] == -1 ) break;
         }

         // Out of clients
         if ( x == MaxClients_ ) {
            if ( debug_ ) cout << "ControlServer::receive -> Rejected connection" << endl;
            close(newFd);
         }
         else {
            if ( debug_ ) cout << "ControlServer::receive -> Accepted new connection" << endl;
            connFd_[x] = newFd;

            msg.str("");
            msg << "<system>" << endl;
            msg << system_->structureString(false);
            msg << system_->configString(false);
            msg << system_->statusString(false);
            msg << "</system>" << endl;
            msg << "\f";
            write(connFd_[x],msg.str().c_str(),msg.str().length());
         }
      }
   }

   // client socket is ready
   for ( x=0; x < MaxClients_; x++ ) {
      if ( ret > 0 && connFd_[x] >= 0 && FD_ISSET(connFd_[x],&fdset) ) {

         // Read data
         ret = read(connFd_[x],buffer,9000);

         // Connection is lost
         if ( ret <= 0 ) {
            if ( debug_ ) cout << "ControlServer::receive -> Closing connection" << endl;

            // Reset
            rxData_[x].str("");

            // Close socket
            close(connFd_[x]);
            connFd_[x] = -1;
         }

         // Process each byte
         for (y=0; y<ret; y++) {
            if ( buffer[y] == '\f' || buffer[y] == 0x4 ) {

               // Send to top level
               if ( system_ != NULL ) {
                  if ( debug_ ) {
                     cout << "ControlServer::receive -> Processing message: " << endl;
                     cout << rxData_[x].str() << endl;
                  }
                  system_->parseXmlString (rxData_[x].str()); 
                  if ( debug_ ) cout << "ControlServer::receive -> Done Processing message" << endl;
               }
               rxData_[x].str("");
            }
            else rxData_[x] << buffer[y];
         }
      }
   }

   // Shared memory commands
   if ( controlCmdPending(smem_) ) {
      if ( debug_ ) cout << "ControlServer::receive -> Processing shared memory message: " << endl;
      system_->parseXmlString(controlCmdBuffer(smem_));
      cmdPend = true;
   }
   else cmdPend = false;

   // Poll
   pmsg = system_->poll();

   // Copy resulting system state to buffer
   if ( cmdPend ) {
      strcpy(controlStatBuffer(smem_),system_->get("SystemState").c_str());
      strcpy(controlUserBuffer(smem_),system_->get("UserStatus").c_str());
      controlCmdAck(smem_);
      if ( debug_ ) cout << "CodaServer::receive -> Done Processing shared memory message: " << endl;
      cmdPend = false;
   }

   // Send message
   if ( pmsg != "" ) {
      msg.str("");
      msg << pmsg;
      msg << "\f";

      for ( x=0; x < MaxClients_; x++ ) {
         if ( connFd_[x] >= 0 ) write(connFd_[x],msg.str().c_str(),msg.str().length());
      }
   }
}

