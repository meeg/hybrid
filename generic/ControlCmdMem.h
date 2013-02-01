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
#include <stdio.h>

#define CONTROL_CMD_SIZE      10240
#define CONTROL_CMD_XML_SIZE  1048576
#define CONTROL_CMD_NAME_SIZE 200

typedef struct {

   // Command control
   char         cmdRdyCount;
   char         cmdAckCount;
   char         cmdBuffer[CONTROL_CMD_SIZE];
   char         errorBuffer[CONTROL_CMD_SIZE];
   char         statBuffer[CONTROL_CMD_SIZE];
   char         userBuffer[CONTROL_CMD_SIZE];
   char         xmlStatusBuffer[CONTROL_CMD_XML_SIZE];
   char         xmlConfigBuffer[CONTROL_CMD_XML_SIZE];
   char         sharedName[CONTROL_CMD_NAME_SIZE];

} ControlCmdMemory;

// Open and map shared memory
inline int controlCmdOpenAndMap ( ControlCmdMemory **ptr, const char *system, unsigned int id ) {
   int           smemFd;
   char          shmName[200];

   // Generate shared memory
   sprintf(shmName,"control_cmd.%i.%s.%i",getuid(),system,id);

   // Attempt to open existing shared memory
   if ( (smemFd = shm_open(shmName, O_RDWR, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) {

      // Otherwise open and create shared memory
      if ( (smemFd = shm_open(shmName, (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ) < 0 ) return(-1);

      // Force permissions regardless of umask
      fchmod(smemFd, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
    
      // Set the size of the shared memory segment
      ftruncate(smemFd, sizeof(ControlCmdMemory));
   }

   // Map the shared memory
   if((*ptr = (ControlCmdMemory *)mmap(0, sizeof(ControlCmdMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd, 0)) == MAP_FAILED) return(-2);

   // Store name
   strcpy((*ptr)->sharedName,shmName);

   return(smemFd);
}

// Close shared memory
inline void controlCmdClose ( ControlCmdMemory *ptr ) {
   char shmName[200];

   // Get shared name
   strcpy(shmName,ptr->sharedName);

   // Unlink
   shm_unlink(shmName);
}

// Init data structure, called by ControlServer
inline void controlCmdInit ( ControlCmdMemory *ptr ) {
   memset(ptr->cmdBuffer, 0, CONTROL_CMD_SIZE);
   memset(ptr->cmdBuffer, 0, CONTROL_CMD_SIZE);
   memset(ptr->errorBuffer, 0, CONTROL_CMD_SIZE);
   memset(ptr->statBuffer, 0, CONTROL_CMD_SIZE);
   memset(ptr->userBuffer, 0, CONTROL_CMD_SIZE);
   memset(ptr->xmlStatusBuffer, 0, CONTROL_CMD_XML_SIZE);
   memset(ptr->xmlConfigBuffer, 0, CONTROL_CMD_XML_SIZE);

   ptr->cmdRdyCount = 0;
   ptr->cmdAckCount = 0;

}

// Return pointer to command buffer
inline char * controlCmdBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->cmdBuffer);
}

// Send cmd, called by client
inline void controlCmdSend ( ControlCmdMemory *ptr, const char *cmd ) {
   strcpy(ptr->errorBuffer,"");
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

// Return pointer to status buffer
inline char * controlStatBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->statBuffer);
}

// Return pointer to user buffer
inline char * controlUserBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->userBuffer);
}

// Return pointer to xml configuration
inline char * controlXmlConfigBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->xmlConfigBuffer);
}

// Return pointer to xml status
inline char * controlXmlStatusBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->xmlStatusBuffer);
}

// Return pointer to error buffer
inline char * controlErrorBuffer ( ControlCmdMemory *ptr ) {
   return(ptr->errorBuffer);
}

#endif

