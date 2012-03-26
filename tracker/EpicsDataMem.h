//-----------------------------------------------------------------------------
// File          : EpicsMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 03/23/2012
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// Interface Server
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 03/23/2012: created
//-----------------------------------------------------------------------------
#ifndef __EPICS_DATA_MEM_H__
#define __EPICS_DATA_MEM_H__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>

#define EPICS_DATA_COUNT  128
#define EPICS_DATA_FILE   "/epics_mem.shared"

typedef struct {

   // Epics control
   unsigned int dataRdyCount;
   unsigned int dataAckCount;
   double       epicsData[EPICS_DATA_COUNT];

} EpicsDataMem;

// Open and map shared memory
inline int epicsDataOpenAndMap ( EpicsDataMem **ptr ) {
   int smemFd;

   // Open shared memory
   smemFd = shm_open(EPICS_DATA_FILE, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));

   // Failed to open shred memory
   if ( smemFd < 0 ) return(-1);
  
   // Force permissions regardless of umask
   fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
 
   // Set the size of the shared memory segment
   ftruncate(smemFd, sizeof(EpicsDataMem));

   // Map the shared memory
   if((*ptr = (EpicsDataMem *)mmap(0, sizeof(EpicsDataMem),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   return(smemFd);
}

// Init data structure
inline void epicsDataInit ( EpicsDataMem *ptr ) {
   int x;

   ptr->dataRdyCount = 0;
   ptr->dataAckCount = 0;
   for (x=0; x < EPICS_DATA_COUNT; x++) ptr->epicsData[x] = 0.0;
}

// Set epics value
inline void epicsDataSet ( EpicsDataMem *ptr, int index, double value ) {
   if ( index < EPICS_DATA_COUNT ) ptr->epicsData[index] = value;
}

// Get epics value
inline double epicsDataGet ( EpicsDataMem *ptr, int index ) {
   if ( index < EPICS_DATA_COUNT ) return(ptr->epicsData[index]);
   else return(0);
}

// Set data ready
inline void epicsDataSetReady (EpicsDataMem *ptr) {
   ptr->dataRdyCount++;
}

// Get data ready
inline int epicsDataGetReady (EpicsDataMem *ptr) {
   if ( ptr->dataRdyCount != ptr->dataAckCount ) {
      ptr->dataAckCount = ptr->dataRdyCount;
      return(1);
   }
   else return(0);
}

#endif

