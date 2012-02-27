//-----------------------------------------------------------------------------
// File          : TrackerDataMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/11/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Shared memory used by TrackerLink class.
//
// Each data buffer is for a single FPGA's worth of data:
// Header = 8 x 32-bits
// 
// Sample = 4 * 3 hybrids * 5 APVs * 128 channels x 32-bits
//        = 7680 x 32-bits
// 
// Tail   = 1 x 32-bits
//
// Total size = 7689 x 32-bits
//
// Total data count. 8 channels x 8 entries + extras for other received.
// Chosen total = 72
//
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 02/18/2012: created
//-----------------------------------------------------------------------------
#ifndef __TRACKER_DATA_MEM_H__
#define __TRACKER_DATA_MEM_H__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>

#define TRACKER_DATA_SIZE  8192
#define TRACKER_DATA_COUNT 512
#define TRACKER_DATA_RX    8
#define TRACKER_DATA_FILE  "/tracker_data.shared"

typedef struct {

   // Data Buffers
   unsigned int dataBuffer[TRACKER_DATA_COUNT][TRACKER_DATA_SIZE];
   unsigned int dataBufferSize[TRACKER_DATA_COUNT];

   // Data RX Buffers
   unsigned int dataRxList[TRACKER_DATA_RX][TRACKER_DATA_COUNT];

   // Data Rx Pointers
   unsigned int dataRxWrite[TRACKER_DATA_RX];
   unsigned int dataRxRead[TRACKER_DATA_RX];

   // Free List
   unsigned int freeList[TRACKER_DATA_COUNT];

   // Free List Pointers
   unsigned int freeListWrite;
   unsigned int freeListRead;

   // Ack tracking counters
   unsigned int ackSendCount;
   unsigned int ackSentCount;

   // Listener is active flag
   unsigned int listenActive;

} TrackerDataMemory;

// Open and map shared memory
inline int trackerDataOpenAndMap ( TrackerDataMemory **ptr ) {
   int smemFd;

   // Open shared memory
   smemFd = shm_open(TRACKER_DATA_FILE, (O_CREAT | O_RDWR), (S_IREAD | S_IWRITE));

   // Failed to open shred memory
   if ( smemFd < 0 ) return(-1);
   
   // Set the size of the shared memory segment
   ftruncate(smemFd, sizeof(TrackerDataMemory));

   // Map the shared memory
   if((*ptr = (TrackerDataMemory *)mmap(0, sizeof(TrackerDataMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   return(smemFd);
}

// Init data structure, called by TrackerLink main thread
inline void trackerDataInit ( TrackerDataMemory *ptr ) {
   memset(ptr->dataBuffer,  0, TRACKER_DATA_COUNT*TRACKER_DATA_SIZE*4);
   memset(ptr->dataRxList,  0, TRACKER_DATA_RX*TRACKER_DATA_COUNT*4);
   memset(ptr->dataRxWrite, 0, TRACKER_DATA_RX*4);
   memset(ptr->dataRxRead,  0, TRACKER_DATA_RX*4);
   memset(ptr->freeList,    0, TRACKER_DATA_COUNT*4);

   ptr->freeListWrite = 0;
   ptr->freeListRead  = 0;
   ptr->ackSendCount  = 0;
   ptr->ackSentCount  = 0;
   ptr->listenActive  = 0;

   // Populate free list
   for (uint x=0; x < (TRACKER_DATA_COUNT-1); x++) {
      ptr->freeList[ptr->freeListWrite] = x;
      ptr->freeListWrite = (ptr->freeListWrite + 1) % TRACKER_DATA_COUNT;
   }
}

// Return pointer to buffer, called by all
inline unsigned int *trackerDataEntry ( TrackerDataMemory *ptr, unsigned int entry ) { 
   return(ptr->dataBuffer[entry]); 
}

// Get entry size
inline unsigned int trackerDataGetSize ( TrackerDataMemory *ptr, unsigned int entry ) { 
   return(ptr->dataBufferSize[entry]);
}

// Set entry size
inline void trackerDataSetSize ( TrackerDataMemory *ptr, unsigned int entry, unsigned int size ) { 
   ptr->dataBufferSize[entry] = size;
}

// Push to free list, called by coda client
inline void trackerDataFreePush ( TrackerDataMemory *ptr, unsigned int entry ) {
   ptr->freeList[ptr->freeListWrite] = entry;
   ptr->freeListWrite = (ptr->freeListWrite + 1) % TRACKER_DATA_COUNT;
}

// Pop from free list, called by TrackerLink receive/transmit thread
inline int trackerDataFreePop ( TrackerDataMemory *ptr ) {
   unsigned int next;
   unsigned int entry;

   if ( ptr->freeListRead == ptr->freeListWrite ) return -1;
   next = (ptr->freeListRead + 1) % TRACKER_DATA_COUNT;
   entry = ptr->freeList[ptr->freeListRead];
   ptr->freeListRead = next;
   return entry;
}

// Push to rx list, called by TrackerLink receive/transmit thread
inline void trackerDataRxPush ( TrackerDataMemory *ptr, unsigned int list, unsigned int entry ) {
   if ( list < TRACKER_DATA_RX ) {
      ptr->dataRxList[list][ptr->dataRxWrite[list]] = entry;
      ptr->dataRxWrite[list] = (ptr->dataRxWrite[list] + 1) % TRACKER_DATA_COUNT;
   }
}

// Pop from rx list, called by Coda client
inline int trackerDataRxPop ( TrackerDataMemory *ptr, unsigned int list ) {
   unsigned int next;
   unsigned int entry;

   if ( list >= TRACKER_DATA_RX ) return(-1);
   if ( ptr->dataRxRead[list] == ptr->dataRxWrite[list] ) return -1;

   next = (ptr->dataRxRead[list] + 1) % TRACKER_DATA_COUNT;
   entry = ptr->dataRxList[list][ptr->dataRxRead[list]];
   ptr->dataRxRead[list] = next;
   return entry;
}

// Get entry count for rx list, called by Coda client
inline unsigned int trackerDataRxCount ( TrackerDataMemory *ptr, unsigned int list ) {
   if ( list >= TRACKER_DATA_RX ) return(0);

   if ( ptr->dataRxRead[list] == ptr->dataRxWrite[list] ) return 0;

   if ( ptr->dataRxWrite[list] > ptr->dataRxRead[list] ) 
      return(ptr->dataRxWrite[list] - ptr->dataRxRead[list]); 

   else return(ptr->dataRxWrite[list] + (TRACKER_DATA_COUNT - ptr->dataRxRead[list]));
}

// Send ack, called by Coda client
inline void trackerDataSendAck ( TrackerDataMemory *ptr ) {
   ptr->ackSendCount++;
}

// Ack needed check, called by TrackerLink receive/transmit thread
inline int trackerDataNeedAck ( TrackerDataMemory *ptr ) {
   if ( ptr->ackSendCount != ptr->ackSentCount ) return(1);
   else return(0);
}

// Ack sent, called by TrackerLink receive/transmit thread
inline void trackerDataSentAck ( TrackerDataMemory *ptr ) {
   ptr->ackSentCount = ptr->ackSendCount;
}

// Set listen active flag
inline void trackerDataSetListenActive ( TrackerDataMemory *ptr, unsigned int state ) {
   ptr->listenActive = state;
}

// Set listen active flag
inline unsigned int trackerDataGetListenActive ( TrackerDataMemory *ptr ) {
   return(ptr->listenActive);
}

#endif

