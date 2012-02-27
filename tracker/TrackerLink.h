//-----------------------------------------------------------------------------
// File          : TrackerLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP communications link for tracker
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 02/19/2012: created
//-----------------------------------------------------------------------------
#ifndef __TRACKER_LINK_H__
#define __TRACKER_LINK_H__

#include <sys/types.h>
#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
#include <TrackerDataMem.h>

using namespace std;

//! Class to contain PGP communications link
class TrackerLink : public CommLink {

   protected:

      // Shared memory pointers
      int               shmFd_;
      TrackerDataMemory *shm_;

      // Values used for udp version
      uint   udpCount_;
      int    *udpFd_;
      struct sockaddr_in *udpAddr_;

      // Data routine
      void dataHandler();

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      TrackerLink ( );

      //! Deconstructor
      ~TrackerLink ( );

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param port  udp port
       * \param count host count
       * \param host  udp hosts
      */
      void open ( int port, uint count, ... );

      //! Stop threads and close link
      void close ();

};
#endif
