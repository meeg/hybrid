//-----------------------------------------------------------------------------
// File          : OnlineDataMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/15/2012
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// Online viewer shared memory
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 03/23/2012: created
//-----------------------------------------------------------------------------
#ifndef __ONLINE_DATA_MEM_H__
#define __ONLINE_DATA_MEM_H__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>

#define ONLINE_DATA_COUNT 1024
#define ONLINE_DATA_SIZE  8192
#define ONLINE_DATA_FILE   "/online_mem.shared"

typedef struct {

   // Epics control
   unsigned int dataRdPtr;
   unsigned int dataWrPtr;
   unsigned int onlineData[ONLINE_DATA_COUNT][ONLINE_DATA_SIZE];
   unsigned int onlineSize[ONLINE_DATA_COUNT];

} OnlineDataMem;

// Open and map shared memory
inline int onlineDataOpenAndMap ( OnlineDataMem **ptr ) {
   int smemFd;

   // Open shared memory
   smemFd = shm_open(ONLINE_DATA_FILE, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));

   // Failed to open shred memory
   if ( smemFd < 0 ) return(-1);
  
   // Force permissions regardless of umask
   fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
 
   // Set the size of the shared memory segment
   ftruncate(smemFd, sizeof(OnlineDataMem));

   // Map the shared memory
   if((*ptr = (OnlineDataMem *)mmap(0, sizeof(OnlineDataMem),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   return(smemFd);
}

// Init data structure
inline void onlineDataInit ( OnlineDataMem *ptr ) {
   ptr->dataRdPtr = 0;
   ptr->dataWrPtr = 0;
}

// Push To Queue
inline void onlineDataPush ( OnlineDataMem *ptr, unsigned int *data, unsigned int size) {
   unsigned int next;

   next = (ptr->dataWrPtr + 1) % ONLINE_DATA_COUNT;
   if ( size <= ONLINE_DATA_SIZE && next != ptr->dataRdPtr ) {
      memcpy(ptr->onlineData[ptr->dataWrPtr],data,size*4);
      ptr->onlineSize[ptr->dataWrPtr] = size;
      ptr->dataWrPtr = next;
   }
}

// Pop from queue
inline unsigned int onlineDataPop ( OnlineDataMem *ptr, unsigned int **data ) {
   unsigned int next;
   unsigned int size;

   if ( ptr->dataRdPtr == ptr->dataWrPtr ) return 0;
   next = (ptr->dataRdPtr + 1) % ONLINE_DATA_COUNT;
   *data = ptr->onlineData[ptr->dataRdPtr];
   size  = ptr->onlineSize[ptr->dataRdPtr];
   ptr->dataRdPtr = next;
   return size;
}

#endif

