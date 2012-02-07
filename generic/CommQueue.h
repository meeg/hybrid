//-----------------------------------------------------------------------------
// File          : CommQueue.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Communications Queue
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __COMM_QUEUE_H__
#define __COMM_QUEUE_H__
using namespace std;

// Class For IBIOS Messages
class CommQueue {

      // Constants
      static const unsigned int size = 1000;

      // Read and write pointer
      volatile unsigned int read;
      volatile unsigned int write;
 
      // Circular Buffer
      void *data[size];

   public:

      // Constructor
      CommQueue();

      // Push single element to queue
      bool push ( void *ptr );

      // Pop single element from queue
      void *pop ( );

      // Queue has data
      bool ready ();      

};
#endif
