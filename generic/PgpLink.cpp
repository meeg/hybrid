//-----------------------------------------------------------------------------
// File          : PgpLink.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP communications link
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <PgpLink.h>
#include <PgpCardMod.h>
#include <PgpCardWrap.h>
#include <sstream>
#include "Register.h"
#include "Command.h"
#include "Data.h"
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
using namespace std;


// Receive Thread
void PgpLink::rxHandler() {
   uint           *rxBuff;
   int            maxFd;
   struct timeval timeout;
   int            ret;
   fd_set         fds;
   Data           *data;
   uint           lane;
   uint           vc;
   uint           eofe;
   uint           fifoErr;
   uint           lengthErr;
   uint           vcMaskRx;
   uint           laneMaskRx;
   uint           vcMask;
   uint           laneMask;

   // Init buffer
   rxBuff = (uint *) malloc(sizeof(uint)*maxRxTx_);

   // While enabled
   while ( runEnable_ ) {

      // Init fds
      FD_ZERO(&fds);
      FD_SET(fd_,&fds);
      maxFd = fd_;

      // Setup timeout
      timeout.tv_sec  = 0;
      timeout.tv_usec = 500;

      // Select
      if ( select(maxFd+1, &fds, NULL, NULL, &timeout) <= 0 ) continue;

      // Data is available
      if ( FD_ISSET(fd_,&fds) ) {

         // Setup and attempt receive
         ret = pgpcard_recv(fd_, rxBuff, maxRxTx_, &lane, &vc, &eofe, &fifoErr, &lengthErr);

         // No data
         if ( ret <= 0 ) continue;

         // Bad size or error
         if ( ret < 4 || eofe || fifoErr || lengthErr ) {
            if ( debug_ ) {
               cout << "PgpLink::ioHandler -> "
                    << "Error in data receive. Rx=" << dec << ret
                    << ", Lane=" << dec << lane << ", Vc=" << dec << vc
                    << ", EOFE=" << dec << eofe << ", FifoErr=" << dec << fifoErr
                    << ", LengthErr=" << dec << lengthErr << endl;
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
                  cout << "PgpLink::rxHandler -> Unuexpected frame received"
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
void PgpLink::ioHandler() {
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
   while ( runEnable_ ) {

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
         pgpcard_send(fd_, runBuff, 4, runLane, runVc);
  
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
         pgpcard_send(fd_, regBuff_, ((regReqWrite_)?regReqEntry_->size()+3:4), regLane, regVc);
 
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
         pgpcard_send(fd_, cmdBuff, 4, cmdLane, cmdVc);

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }
      else usleep(10);
   }

   free(regBuff_);
}

// Constructor
PgpLink::PgpLink ( ) : CommLink() {
   device_   = "";
   fd_       = -1;
}

// Deconstructor
PgpLink::~PgpLink ( ) {
   close();
}

// Open link and start threads
void PgpLink::open ( string device ) {
   stringstream dbg;

   device_ = device; 

   // Open device without blocking
   fd_ = ::open(device.c_str(),O_RDWR | O_NONBLOCK);

   // Debug result
   if ( fd_ < 0 ) {
      dbg.str("");
      dbg << "PgpLink::open -> ";
      if ( fd_ < 0 ) dbg << "Error opening file ";
      else dbg << "Opened device file ";
      dbg << device_ << endl;
      cout << dbg;
      throw(dbg.str());
   }

   // Status
   if ( fd_ < 0 ) throw(dbg.str());
   CommLink::open();
}

// Stop threads and close link
void PgpLink::close () {

   // Close link
   if ( fd_ >= 0 ) {
      CommLink::close();
      usleep(100);
      ::close(fd_);
   }
   fd_ = -1;
}

