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

// IO Thread
void PgpLink::ioHandler() {
   uint      cmdBuff[4];
   uint      runBuff[4];
   uint      lane;
   uint      vc;
   uint      eofe;
   uint      fifoErr;
   uint      lengthErr;
   int       rxRet;
   int       txRet;
   int       cmdRet;
   int       runRet;
   uint      lastReqCnt;
   uint      lastCmdCnt;
   uint      lastRunCnt;
   uint      txLane;
   uint      txVc;
   uint      cmdLane;
   uint      cmdVc;
   uint      runLane;
   uint      runVc;
   bool      txPend;
   Data      *rxData;
   uint      vcMask;
   uint      laneMask;
   uint      vcMaskRx;
   uint      laneMaskRx;

   // While enabled
   lastReqCnt = regReqCnt_;
   lastCmdCnt = cmdReqCnt_;
   lastRunCnt = runReqCnt_;
   txPend     = false;
   while ( runEnable_ ) {

      // Setup and attempt receive
      rxRet = pgpcard_recv(fd_, rxBuff_, maxRxTx_, &lane, &vc, &eofe, &fifoErr, &lengthErr);

      // Data is ready and large enough to be a real packet
      if ( rxRet > 0 ) {

         // An error occured
         if ( rxRet < 4 || eofe || fifoErr || lengthErr ) {
            if ( debug_ ) {
               cout << "PgpLink::ioHandler -> "
                    << "Error in data receive. Rx=" << dec << rxRet
                    << ", Lane=" << dec << lane << ", Vc=" << dec << vc
                    << ", EOFE=" << dec << eofe << ", FifoErr=" << dec << fifoErr
                    << ", LengthErr=" << dec << lengthErr << endl;
            }
            errorCount_++;
         }

         // Frame was valid
         else {

            if ( rxBuff_[rxRet-1] != 0 && debug_ ) 
               cout << "PgpLink::ioHandler -> "
                    << "Bad tail value in data receive. Rx=" << dec << rxRet
                    << ", Tail=" << hex << setw(8) << setfill('0') << rxBuff_[rxRet-1] << endl;

            // Setup mask values
            vcMaskRx   = (0x1 << vc);
            laneMaskRx = (0x1 << lane);
            vcMask     = (dataMask_ & 0xF);
            laneMask   = ((dataMask_ >> 4) & 0xF);

            // Check for data packet
            if ( (vcMaskRx & vcMask) != 0 && (laneMaskRx & laneMask) != 0 ) {
               rxData = new Data(rxBuff_,rxRet);
               if ( ! dataQueue_.push(rxData) ) {
                  unexpCount_++;
                  delete rxData;
               }
            }

            // Data matches outstanding register request
            else if ( txPend && (memcmp(rxBuff_,txBuff_,8) == 0) && ((uint)(rxRet-3) == regReqEntry_->size())) {
               if ( ! regReqWrite_ ) {
                  if ( rxBuff_[rxRet-1] == 0 ) 
                     memcpy(regReqEntry_->data(),&(rxBuff_[2]),(regReqEntry_->size()*4));
                  else memset(regReqEntry_->data(),0xFF,(regReqEntry_->size()*4));
               }
               regReqEntry_->setStatus(rxBuff_[rxRet-1]);
               txPend = false;
               regRespCnt_++;
            }

            // Unexpected frame
            else {
               unexpCount_++;
               if ( debug_ ) {
                  cout << "PgpLink::ioHandler -> Unuexpected frame received"
                       << " Pend=" <<  txPend
                       << " Comp=" << (memcmp(rxBuff_,txBuff_,8))
                       << " ExpSize=" << regReqEntry_->size()
                       << " GotSize=" << (rxRet-3) 
                       << " VcMaskRx=0x" << hex << vcMaskRx
                       << " VcMask=0x" << hex << vcMask
                       << " LaneMaskRx=0x" << hex << laneMaskRx
                       << " LaneMask=0x" << hex << laneMask << endl;
               }
            }
         }
      }
  
      // Register TX is pending
      if ( lastReqCnt != regReqCnt_ ) {

         // Setup tx buffer
         txBuff_[0]  = 0;
         txBuff_[1]  = (regReqWrite_)?0x40000000:0x00000000;
         txBuff_[1] |= regReqEntry_->address() & 0x00FFFFFF;

         // Write has data
         if ( regReqWrite_ ) {
            memcpy(&(txBuff_[2]),regReqEntry_->data(),(regReqEntry_->size()*4));
            txBuff_[regReqEntry_->size()+2]  = 0;
         }

         // Read is always small
         else {
            txBuff_[2]  = regReqEntry_->size()-1;
            txBuff_[3]  = 0;
         }
 
         // Set lane and vc from upper address bits
         txLane = (regReqEntry_->address()>>28) & 0xF;
         txVc   = (regReqEntry_->address()>>24) & 0xF;

         // Send data
         txRet = pgpcard_send(fd_, txBuff_, regReqEntry_->size()+3, txLane, txVc);
         if ( txRet > 0 ) txPend = true;

         // Match request count
         lastReqCnt = regReqCnt_;
      }
      else txRet = 0;

      // Command TX is pending
      if ( lastCmdCnt != cmdReqCnt_ ) {

         // Setup tx buffer
         cmdBuff[0]  = 0;
         cmdBuff[1]  = cmdReqEntry_->opCode() & 0xFF;
         cmdBuff[2]  = 0;
         cmdBuff[3]  = 0;
 
         // Set lane and vc from upper address bits
         cmdLane = (cmdReqEntry_->opCode()>>12) & 0xF;
         cmdVc   = (cmdReqEntry_->opCode()>>8)  & 0xF;

         // Send data
         cmdRet = pgpcard_send(fd_, cmdBuff, 4, cmdLane, cmdVc);

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }
      else cmdRet = 0;

      // Run Command TX is pending
      if ( lastRunCnt != runReqCnt_ ) {

         // Setup tx buffer
         runBuff[0]  = 0;
         runBuff[1]  = runReqEntry_->opCode() & 0xFF;
         runBuff[2]  = 0;
         runBuff[3]  = 0;
 
         // Set lane and vc from upper address bits
         runLane = (runReqEntry_->opCode()>>12) & 0xF;
         runVc   = (runReqEntry_->opCode()>>8)  & 0xF;

         // Send data
         runRet = pgpcard_send(fd_, runBuff, 4, runLane, runVc);

         // Match request count
         lastRunCnt = runReqCnt_;
      }
      else runRet = 0;

      // Pause if nothing was done
      if ( rxRet <= 0 && txRet <= 0 && cmdRet <= 0 && runRet <= 0 ) usleep(1);
   }
   pthread_exit(NULL);
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

