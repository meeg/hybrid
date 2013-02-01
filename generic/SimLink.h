//-----------------------------------------------------------------------------
// File          : SimLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 09/07/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Communications link For Simulation
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 09/07/2012: created
//-----------------------------------------------------------------------------
#ifndef __SIM_LINK_H__
#define __SIM_LINK_H__

#include <sys/types.h>
#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
using namespace std;

// Constant
#define SIM_LINK_BUFF_SIZE 1000000

// Shared memory structure
typedef struct {

   // Upstream
   uint        usReqCount;
   uint        usAckCount;
   uint        usData[SIM_LINK_BUFF_SIZE];
   uint        usSize;
   uint        usVc;
   uint        usEofe;
   uint        usEthMode;
   
   // Downtream
   uint        dsReqCount;
   uint        dsAckCount;
   uint        dsData[SIM_LINK_BUFF_SIZE];
   uint        dsSize;
   uint        dsVc;
   uint        dsEthMode;

} SimLinkMemory;


//! Class to contain PGP communications link
class SimLink : public CommLink {

   protected:

      // Shared memory
      uint          smemFd_;
      SimLinkMemory *smem_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

      // Copy with byte reorder
      static void swapBytes ( uint *data, uint size );

   public:

      //! Constructor
      SimLink ( );

      //! Deconstructor
      ~SimLink ( );

      //! Open link and start threads
      void open ( string system, uint id );

      //! Stop threads and close link
      void close ();

};
#endif
