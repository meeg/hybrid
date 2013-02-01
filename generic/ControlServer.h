//-----------------------------------------------------------------------------
// File          : ControlServer.h
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
#ifndef __CONTROL_SERVER_H__
#define __CONTROL_SERVER_H__
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
#include <ControlCmdMem.h>
using namespace std;

class System;

//! Class to manage control interface
class ControlServer {

      // Number of clients to support
      static const uint MaxClients_ = 8;

      // Debug flag
      bool debug_;

      // Server fdes
      int servFd_;

      // Connection fdes
      int connFd_[MaxClients_];

      // Port number
      int port_;

      // Socket addresses
      struct sockaddr_in servAddr_;
      struct sockaddr_in connAddr_;

      // Current received data
      stringstream rxData_[MaxClients_];

      // Top level device
      System *system_;

      // Shared memory
      uint             smemFd_;
      ControlCmdMemory *smem_;

   public:

      //! Constructor
      ControlServer ();

      //! DeConstructor
      ~ControlServer ();

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
       * \param port Listen port number, pass zero to auto assign
       * resulting port number is returned
      */
      int startListen ( int port );

      //! Stop tcpip listen socket
      void stopListen ( );

      //! Enable shared memory for control
      /*! 
       * \param system System name
       * \param id ID to identify your process
      */
      void enableSharedMemory ( string system, uint id );

      //! Receive and process data if ready
      /*! 
       * \param timeout timeout value in microseconds
      */
      void receive ( uint timeout );
};

#endif
