//-----------------------------------------------------------------------------
// File          : CodaServer.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/11/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// CODA interface server class
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/11/2012: created
//-----------------------------------------------------------------------------
#ifndef __CODA_SERVER_H__
#define __CODA_SERVER_H__
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <CodaMem.h>
#include <CommLink.h>
using namespace std;

//! Class to manage control interface
class CodaServer {

      // Debug flag
      bool debug_;

      // Server fdes
      int servFd_;

      // Connection fdes
      int connFd_;

      // Port number
      int port_;

      // Socket addresses
      struct sockaddr_in servAddr_;
      struct sockaddr_in connAddr_;

      // Current received data
      stringstream rxData_;

      // Top level device
      System *system_;

      // Shared memory
      uint smemFd_;
      TrackerSharedMemory *smem_;

      // Shared memory control
      uint dataReady_[TRACKER_DATA_CHAN];
      uint cmdRdyLast_;

   public:

      //! Constructor
      CodaServer ();

      //! DeConstructor
      ~CodaServer ();

      //! Setup comm link
      void setupCommLink ( CommLink *link );

      //! Set system instance
      /*! 
       * \param system System object
      */
      void setSystem ( System *system );

      //! Set debug flag
      /*! 
       * \param debug  Debug flag
      */
      void setDebug ( bool debug );

      //! Start tcpip listen socket
      /*! 
       * \param port Listen port number
      */
      void startListen ( int port );

      //! Stop tcpip listen socket
      void stopListen ( );

      //! Receive and process data if ready
      /*! 
       * \param timeout timeout value in microseconds
      */
      void receive ( uint timeout );

      //! Real data callback function
      void dataCb (void *data, uint size);

      //! Static data callback function
      static void staticCb (void *data, uint size);
};

#endif
