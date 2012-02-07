//-----------------------------------------------------------------------------
// File          : CommQueue.cpp
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
#include <CommQueue.h>
using namespace std;


// Constructor
CommQueue::CommQueue() {
   read  = 0;
   write = 0;
}


// Push single element to queue
bool CommQueue::push ( void *ptr ) {
   unsigned int next;

   next = (write + 1) % size;
   if ( next != read ) {
      data[write] = ptr;
      write = next;
      return true;
   } else return false;
}


// Pop single element from queue
void *CommQueue::pop ( ) {
   unsigned int next;
   void         *ptr;

   if ( read == write ) return false;
   next = (read + 1) % size;
   ptr = data[read];
   read = next;
   return ptr;
}


// Queue has data
bool CommQueue::ready () {
   return( read != write );
}

