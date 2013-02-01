//-----------------------------------------------------------------------------
// File          : SimLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 09/07/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Communications link for simulation
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 09/07/2012: created
//-----------------------------------------------------------------------------
#include <SimLink.h>
#include <sstream>
#include "Register.h"
#include "Command.h"
#include "Data.h"
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

// Copy with byte reorder
void SimLink::swapBytes ( uint *data, uint size ) {
   uint          dint;
   uint          sint;
   unsigned char *dchar;
   unsigned char *schar;

   dchar = (unsigned char *)(&dint);
   schar = (unsigned char *)(&sint);

   for (uint i=0; i < size; i++) {
      sint = data[i];

      dchar[0] = schar[2];
      dchar[1] = schar[3];
      dchar[2] = schar[0];
      dchar[3] = schar[1];

      data[i] = dint;
   }
}

// Receive Thread
void SimLink::rxHandler() {
   uint           *rxBuff;
   int            ret;
   Data           *data;
   uint           lane;
   uint           vc;
   uint           eofe;
   uint           vcMaskRx;
   uint           laneMaskRx;
   uint           vcMask;
   uint           laneMask;

   // Init buffer
   rxBuff = (uint *) malloc(sizeof(uint)*maxRxTx_);

   // While enabled
   while ( runEnable_ && smem_ != NULL ) {

      // Data is available
      if ( smem_->usReqCount != smem_->usAckCount ) {

         // Too large
         if ( smem_->usSize > maxRxTx_ ) {
            ret  = 0;
            lane = 0;
            vc   = smem_->usVc;
            eofe = 1;
            smem_->usAckCount = smem_->usReqCount;
         }

         // Size is ok
         else {
            memcpy(rxBuff,smem_->usData,(smem_->usSize)*4);
            ret  = smem_->usSize;
            lane = 0;
            vc   = smem_->usVc;
            eofe = smem_->usEofe;
            smem_->usAckCount = smem_->usReqCount;
            if ( smem_->usEthMode ) swapBytes(rxBuff,ret);
         }

         // Bad size or error
         if ( ret < 4 || eofe ) {
            if ( debug_ ) {
               cout << "SimLink::ioHandler -> "
                    << "Error in data receive. Rx=" << dec << ret
                    << ", Lane=" << dec << lane << ", Vc=" << dec << vc
                    << ", EOFE=" << dec << eofe << endl;
            }
            errorCount_++;
            continue;
         }

         // Check for data packet
         vcMaskRx   = (0x1 << vc);
         laneMaskRx = (0x1 << lane);
         vcMask     = (dataMask_ & 0xF);
         laneMask   = ((dataMask_ >> 4) & 0xF);

         if ( (vcMaskRx & vcMask) != 0 && (laneMaskRx & laneMask) != 0 ) {
            data = new Data(rxBuff,ret);
            dataQueue_.push(data);
         }

         // Reformat header for register rx
         else {

            // Data matches outstanding register request
            if ( memcmp(rxBuff,regBuff_,8) == 0 && (uint)(ret-3) == regReqEntry_->size()) {
               if ( ! regReqWrite_ ) {
                  if ( rxBuff[ret-1] == 0 ) 
                     memcpy(regReqEntry_->data(),&(rxBuff[2]),(regReqEntry_->size()*4));
                  else memset(regReqEntry_->data(),0xFF,(regReqEntry_->size()*4));
               }
               regReqEntry_->setStatus(rxBuff[ret-1]);
               regRespCnt_++;
            }

            // Unexpected frame
            else {
               unexpCount_++;
               if ( debug_ ) {
                  cout << "SimLink::rxHandler -> Unuexpected frame received"
                       << " Comp=" << dec << (memcmp(rxBuff,regBuff_,8))
                       << " Word0_Exp=0x" << hex << regBuff_[0]
                       << " Word0_Got=0x" << hex << rxBuff[0]
                       << " Word1_Exp=0x" << hex << regBuff_[1]
                       << " Word1_Got=0x" << hex << rxBuff[1]
                       << " ExpSize=" << dec << regReqEntry_->size()
                       << " GotSize=" << dec << (ret-3) 
                       << " VcMaskRx=0x" << hex << vcMaskRx
                       << " VcMask=0x" << hex << vcMask
                       << " LaneMaskRx=0x" << hex << laneMaskRx
                       << " LaneMask=0x" << hex << laneMask << endl;
               }
            }
         }
      }
   }

   free(rxBuff);
}

// Transmit thread
void SimLink::ioHandler() {
   uint           cmdBuff[4];
   uint           runBuff[4];
   uint           lastReqCnt;
   uint           lastCmdCnt;
   uint           lastRunCnt;
   uint           runVc;
   uint           runLane;
   uint           regVc;
   uint           regLane;
   uint           cmdVc;
   uint           cmdLane;
   
   // Setup
   lastReqCnt = regReqCnt_;
   lastCmdCnt = cmdReqCnt_;
   lastRunCnt = runReqCnt_;

   // Init register buffer
   regBuff_ = (uint *) malloc(sizeof(uint)*maxRxTx_);

   // While enabled
   while ( runEnable_ && smem_ != NULL ) {

      // Run Command TX is pending
      if ( lastRunCnt != runReqCnt_ ) {

         // Setup tx buffer
         runBuff[0]  = 0;
         runBuff[1]  = runReqEntry_->opCode() & 0xFF;
         runBuff[2]  = 0;
         runBuff[3]  = 0;
 
         // Setup transmit
         runLane = (runReqEntry_->opCode()>>12) & 0xF;
         runVc   = (runReqEntry_->opCode()>>8)  & 0xF;
        
         // Send data
         smem_->dsSize = 4;
         smem_->dsVc   = runVc;
         memcpy(smem_->dsData,runBuff,(smem_->dsSize)*4);
         if ( smem_->dsEthMode ) swapBytes(smem_->dsData,smem_->dsSize);
         smem_->dsReqCount++;
         while (smem_->dsReqCount != smem_->dsAckCount) usleep(100);

         // Match request count
         lastRunCnt = runReqCnt_;
      }

      // Register TX is pending
      else if ( lastReqCnt != regReqCnt_ ) {

         // Setup tx buffer
         regBuff_[0]  = 0;
         regBuff_[1]  = (regReqWrite_)?0x40000000:0x00000000;
         regBuff_[1] |= regReqEntry_->address() & 0x00FFFFFF;

         // Write has data
         if ( regReqWrite_ ) {
            memcpy(&(regBuff_[2]),regReqEntry_->data(),(regReqEntry_->size()*4));
            regBuff_[regReqEntry_->size()+2]  = 0;
         }

         // Read is always small
         else {
            regBuff_[2]  = (regReqEntry_->size()-1);
            regBuff_[3]  = 0;
         }

         // Set lane and vc from upper address bits
         regLane = (regReqEntry_->address()>>28) & 0xF;
         regVc   = (regReqEntry_->address()>>24) & 0xF;

         // Send data
         smem_->dsSize = ((regReqWrite_)?regReqEntry_->size()+3:4);
         smem_->dsVc   = regVc;
         memcpy(smem_->dsData,regBuff_,(smem_->dsSize)*4);
         if ( smem_->dsEthMode ) swapBytes(smem_->dsData,smem_->dsSize);
         smem_->dsReqCount++;
         while (smem_->dsReqCount != smem_->dsAckCount) usleep(100);

         // Match request count
         lastReqCnt = regReqCnt_;
      }

      // Command TX is pending
      else if ( lastCmdCnt != cmdReqCnt_ ) {

         // Setup tx buffer
         cmdBuff[0]  = 0;
         cmdBuff[1]  = cmdReqEntry_->opCode() & 0xFF;
         cmdBuff[2]  = 0;
         cmdBuff[3]  = 0;

         // Setup transmit
         cmdLane = (cmdReqEntry_->opCode()>>12) & 0xF;
         cmdVc   = (cmdReqEntry_->opCode()>>8)  & 0xF;
        
         // Send data
         smem_->dsSize = 4;
         smem_->dsVc   = cmdVc;
         memcpy(smem_->dsData,cmdBuff,(smem_->dsSize)*4);
         if ( smem_->dsEthMode ) swapBytes(smem_->dsData,smem_->dsSize);
         smem_->dsReqCount++;
         while (smem_->dsReqCount != smem_->dsAckCount) usleep(100);

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }
      else usleep(10);
   }

   free(regBuff_);
}

// Constructor
SimLink::SimLink ( ) : CommLink() {
   smemFd_    = -1;
   smem_      = NULL;
   toDisable_ = true;
}

// Deconstructor
SimLink::~SimLink ( ) {
   close();
}

// Open link and start threads
void SimLink::open ( string system, uint id ) {
   stringstream tmp;

   smem_ = NULL;

   // Generate file name
   tmp.str("");
   tmp << "simlink." << dec << setw(1) << getuid() << "." << system << "." << dec << id;

   // Debug
   cout << "SimLink::open -> Using shared memory file " << tmp.str() << endl;

   // Open shared memory
   smemFd_ = shm_open(tmp.str().c_str(), (O_CREAT | O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));

   // Failed to open shred memory
   if ( smemFd_ < 0 ) throw string("SimLink::open -> Could Not Open Shared Memory");
  
   // Force permissions regardless of umask
   fchmod(smemFd_, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH));
 
   // Set the size of the shared memory segment
   ftruncate(smemFd_, sizeof(SimLinkMemory));

   // Map the shared memory
   if((smem_ = (SimLinkMemory *)mmap(0, sizeof(SimLinkMemory),
              (PROT_READ | PROT_WRITE), MAP_SHARED, smemFd_, 0)) == MAP_FAILED) {
      smemFd_ = -1;
      smem_   = NULL;
      if ( smemFd_ < 0 ) throw string("SimLink::open -> Could Not Map Shared Memory");
   }

   // Init records
   smem_->usReqCount = 0;
   smem_->usAckCount = 0;
   smem_->dsReqCount = 0;
   smem_->dsAckCount = 0;

   CommLink::open();
}

// Stop threads and close link
void SimLink::close () {
   ::close(smemFd_);
   smemFd_ = -1;
   smem_   = NULL;
}

