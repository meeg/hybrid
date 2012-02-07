//-----------------------------------------------------------------------------
// File          : CodaMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/11/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// CODA Interface Server
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/11/2012: created
//-----------------------------------------------------------------------------
#ifndef __CODA_MEM_H__
#define __CODA_MEM_H__

#define TRACKER_DATA_SIZE 32768
#define TRACKER_DATA_CHAN 8
#define TRACKER_CMD_SIZE  10240
#define TRACKER_MEM_FILE  "/tracker.shared"

typedef struct {

   // Command control
   unsigned int cmdRdyCount;                 // Incremented by CODA ROC
   unsigned int cmdAckCount;                 // Incremented by control server
   char         cmdBuffer[TRACKER_CMD_SIZE]; // Written by CODA ROC

   // Data Control
   unsigned int dataEnableMask;                                   // Written by control server
   unsigned int dataRdyCount;                                     // Incremented by control server
   unsigned int dataAckCount;                                     // Incremented by CODA ROC
   unsigned int dataSize[TRACKER_DATA_CHAN];                      // Written by control server
   unsigned int dataBuffer[TRACKER_DATA_CHAN][TRACKER_DATA_SIZE]; // Written by control server

   // Future status path?

} TrackerSharedMemory;

#endif
