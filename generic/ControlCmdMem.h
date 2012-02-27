//-----------------------------------------------------------------------------
// File          : ControlCmdMem.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/11/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Interface Server
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/11/2012: created
//-----------------------------------------------------------------------------
#ifndef __CONTROL_CMD_MEM_H__
#define __CONTROL_CMD_MEM_H__
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>   
#include <unistd.h>

#define CONTROL_CMD_SIZE   10240
#define CONTROL_CMD_FILE   "/control_cmd.shared"

typedef struct {

   // Command control
   unsigned int cmdRdyCount;
   unsigned int cmdAckCount;
   char         cmdBuffer[CONTROL_CMD_SIZE];

} ControlCmdMemory;

// Open and map shared memory
inline int controlCmdOpenAndMap ( ControlCmdMemory **ptr ) {
   int smemFd;

   // Open shared memory
   smemFd = shm_open(CONTROL_CMD_FILE, (O_CREAT | O_RDWR), (S_IREAD | S_IWRITE));

   // Failed to open shred memory
   if ( smemFd < 0 ) return(-1);
   
   // Set the size of the shared memory segment
   ftruncate(smemFd, sizeof(ControlCmdMemory));

   // Map the shared memory
   if((*ptr = (ControlCmdMemory *)mmap(0, sizeof(ControlCmdMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   return(smemFd);
}

// Init data structure, called by ControlServer
inline void controlCmdInit ( ControlCmdMemory *ptr ) {
   memset(ptr->cmdBuffer, 0, CONTROL_CMD_SIZE);

   ptr->cmdRdyCount = 0;
   ptr->cmdAckCount = 0;
}

// Return pointer to command buffer
inline char * controlCmdBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->cmdBuffer);
}

// Send cmd, called by client
inline void controlCmdSend ( ControlCmdMemory *ptr, const char *cmd ) {
   strcpy(ptr->cmdBuffer,cmd);
   ptr->cmdRdyCount++;
}

// Cmd pending check, called by either
inline int controlCmdPending ( ControlCmdMemory *ptr ) {
   if ( ptr->cmdRdyCount != ptr->cmdAckCount ) return(1);
   else return(0);
}

// Command ack, called by ControlServer
inline void controlCmdAck ( ControlCmdMemory *ptr ) {
   ptr->cmdAckCount = ptr->cmdRdyCount;
}

#endif

