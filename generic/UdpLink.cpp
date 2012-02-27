//-----------------------------------------------------------------------------
// File          : UdpLink.cpp
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
#include <UdpLink.h>
#include <sstream>
#include "Register.h"
#include "Command.h"
#include "Data.h"
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
using namespace std;

// UDP receiver
int UdpLink::udpRx (uint *idx, uint *buffer, uint size, uint *vc, uint *err ) {
   ulong           rcount;
   ulong           toCount;
   int             ret;
   uint            udpAddrLength;
   unsigned char   qbuffer[9000];
   uint            *quint;
   bool            rSof;
   bool            rEof;
   uint            udpx;
   uint            udpcnt;
   uint            x;

   // Setup length
   udpAddrLength = sizeof(struct sockaddr_in);

   // Keep looping
   toCount = 0;
   rcount  = 0;
   do {

      // Not in frame, attempt every host
      if ( rcount == 0 ) {

         // Attempt each host
         for ((*idx)=0; (*idx) < udpCount_; (*idx)++) {
            if ( udpFd_[*idx] > 0 ) 
               ret = recvfrom(udpFd_[*idx],&qbuffer,9000,0,(struct sockaddr *)(&(udpAddr_[*idx])),&udpAddrLength);
            if ( ret > 0 ) break;
         }

         // No data from any host
         if ( ret <= 0 ) return(0);
      }

      // In frame, only receive from current host
      else ret = recvfrom(udpFd_[*idx],&qbuffer,9000,0,(struct sockaddr *)(&(udpAddr_[*idx])),&udpAddrLength);

      // Got data
      if ( ret > 0 ) {

         // Get each block
         udpx = 0;
         while ( udpx < (uint)ret ) {
            rSof    = (qbuffer[udpx] >> 7) & 0x1;
            rEof    = (qbuffer[udpx] >> 6) & 0x1;
            *vc     = (qbuffer[udpx] >> 4) & 0x3;
            udpcnt  = (qbuffer[udpx] << 8) & 0xF00;
            udpx++;
            udpcnt += (qbuffer[udpx]     ) & 0xFF;
            udpcnt -= 1;
            udpx++;
/*
            cout << "Got Frame."
                 << " idx=" << dec << *idx
                 << " SOF=" << dec << rSof
                 << " EOF=" << dec << rEof
                 << " vc=" << dec << *vc
                 << " udpCnt=" << dec << udpcnt << endl;
*/
            // Bad size
            if ( udpcnt > 4001 ) break;

            // Setup integer pointer
            quint = (uint *)(&(qbuffer[udpx]));

            // Copy payload integer by integer
            for ( x=0; x < (udpcnt/2); x++ ) {
               buffer[rcount] = ntohl(quint[x]);
               rcount++;
            }
            udpx += (udpcnt*2);

         }
      }
      else {
         toCount++;
         if ( toCount >= 100000 ) {
            *err = 1;
            return(0);
         }
         usleep(10);
      }
   } while ( rcount > 0 && ! rEof );
   *err = 0;
   return(rcount);
}

// UDP transmitter
int UdpLink::udpTx (uint idx, uint *buffer, uint size, uint vc ) {
   unsigned char byteData[9000];
   uint          x;
   int           ret;
   uint          usize;
   uint          *uintData;
   uint          txcnt;
   uint          tsize;

   if ( idx >= udpCount_ ) return(0);

   txcnt = 0;
   while ( txcnt < size ) {

      if ( ((size-txcnt) * 4) > 8000 ) {
         usize = 4000;
         tsize = 2000;
      }
      else {
         usize = (size-txcnt) * 2;
         tsize = (size-txcnt);
      }

      // Setup header
      byteData[0]  = 0;
      if ( txcnt == 0 ) byteData[0] |= 0x80; // SOF
      if ( (txcnt + tsize) >= size ) byteData[0] |= 0x40; // EOF
      byteData[0] += (vc << 4) & 0x30;
      byteData[0] += (usize >> 8) & 0xF;
      byteData[1]  = usize & 0xFF;

      // Setup integer pointer
      uintData = (uint *)(&(byteData[2]));

      // Copy payload integer by integer
      for ( x=0; x < tsize; x++ ) uintData[x] = htonl(buffer[txcnt+x]);

      if (udpFd_[idx] > 0 ) 
         ret = sendto(udpFd_[idx],byteData,(tsize*4)+2,0,(struct sockaddr *)(&(udpAddr_[idx])),sizeof(struct sockaddr_in));

      txcnt += tsize;
   }
   return(ret);
}

// IO Thread
void UdpLink::ioHandler() {
   uint      cmdBuff[4];
   uint      runBuff[4];
   uint      vc;
   uint      err;
   int       rxRet;
   int       txRet;
   int       cmdRet;
   int       runRet;
   uint      lastReqCnt;
   uint      lastCmdCnt;
   uint      lastRunCnt;
   uint      txVc;
   uint      cmdVc;
   uint      runVc;
   bool      txPend;
   Data      *rxData;
   uint      rxMask;
   uint      rxIdx;

   // While enabled
   lastReqCnt = regReqCnt_;
   lastCmdCnt = cmdReqCnt_;
   lastRunCnt = runReqCnt_;
   txPend     = false;

   while ( runEnable_ ) {

      // Setup and attempt receive
      rxRet = udpRx(&rxIdx, rxBuff_, maxRxTx_, &vc, &err);
      rxMask  = 1 << vc;

      // Data is ready and large enough to be a real packet
      if ( rxRet > 0 ) {

         // An error occured
         if ( rxRet < 4 || err ) {
            if ( debug_ ) {
               cout << "UdpLink::ioHandler -> "
                    << "Error in data receive. Rx=" << dec << rxRet
                    << ", Vc=" << dec << vc << ", Err=" << dec << err << endl;
            }
            errorCount_++;
         }

         // Frame was valid
         else {

            if ( rxBuff_[rxRet-1] != 0 && debug_ ) 
               cout << "UdpLink::ioHandler -> "
                    << "Bad tail value in data receive. Rx=" << dec << rxRet
                    << ", Tail=" << hex << setw(8) << setfill('0') << rxBuff_[rxRet-1] << endl;

            // Check for data packet
            if ( (dataMask_ & rxMask) != 0 ) {
               rxData = new Data(rxBuff_,rxRet);
               dataQueue_.push(rxData);
            }

            // Data matches outstanding register request
            else if ( txPend && memcmp(rxBuff_,txBuff_,8) == 0 && (uint)(rxRet-3) == regReqEntry_->size()) {
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
                  cout << "UdpLink::ioHandler -> Unuexpected frame received"
                       << " Pend=" <<  txPend
                       << " Comp=" << dec << (memcmp(rxBuff_,txBuff_,8))
                       << " Word0 Exp 0x" << hex << txBuff_[0]
                       << " Word0 Got 0x" << hex << rxBuff_[0]
                       << " Word1 Exp 0x" << hex << txBuff_[1]
                       << " Word1 Got 0x" << hex << rxBuff_[1]
                       << " ExpSize=" << dec << regReqEntry_->size()
                       << " GotSize=" << dec << (rxRet-3) 
                       << " DataMaskRx=0x" << hex << rxMask
                       << " DataMask=0x" << hex << dataMask_ << endl;
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
            txBuff_[2]  = (regReqEntry_->size()-1) & 0x3FF;
            txBuff_[2] |= (txBuff_[2] << 16);
            txBuff_[3]  = 0;
         }
 
         // Set vc from upper address bits
         txVc   = (regReqEntry_->address()>>24) & 0xF;

         // Send data
         txRet = udpTx(regReqDest_,txBuff_, (regReqWrite_)?regReqEntry_->size()+3:4, txVc);
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
 
         // Set vc from upper address bits
         cmdVc   = (cmdReqEntry_->opCode()>>8)  & 0xF;

         // Send data
         cmdRet = udpTx(cmdReqDest_,cmdBuff, 4, cmdVc);

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
 
         // Set vc from upper address bits
         runVc   = (runReqEntry_->opCode()>>8)  & 0xF;

         // Send data
         runRet = udpTx(runReqDest_,runBuff, 4, runVc);

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
UdpLink::UdpLink ( ) : CommLink() {
   udpFd_    = NULL;
   udpAddr_  = NULL;
   udpCount_ = 0;
}

// Deconstructor
UdpLink::~UdpLink ( ) {
   close();
   free(udpAddr_);
}

// Open link and start threads
void UdpLink::open ( int port, uint count, ... ) {
   struct addrinfo*    aiList=0;
   struct addrinfo     aiHints;
   const  sockaddr_in* addr;
   int                 error;
   uint                size;
   int                 flags;
   uint                x;
   const char *        host;

   if ( udpFd_ != NULL ) return;

   // Allocate memory
   udpFd_    = (int *) malloc(count * sizeof(int));
   udpAddr_  = (sockaddr_in *)malloc(count * sizeof(struct sockaddr_in));
   udpCount_ = count;

   // Get list
   va_list a_list;
   va_start(a_list,count);

   // Set each destination
   for (x =0; x < count; x++) { 
      host = va_arg(a_list,const char *);

      // Create socket
      udpFd_[x] = socket(AF_INET,SOCK_DGRAM,0);
      if ( udpFd_[x] == -1 ) throw string("UdpLink::open -> Could Not Create Socket");

      // Set receive size
      size = 2000000;
      setsockopt(udpFd_[x], SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));

      // set non blocking
      flags = fcntl(udpFd_[x],F_GETFL);
      flags |= O_NONBLOCK;
      fcntl(udpFd_[x], F_SETFL, flags);

      // Lookup host address
      aiHints.ai_flags    = AI_CANONNAME;
      aiHints.ai_family   = AF_INET;
      aiHints.ai_socktype = SOCK_DGRAM;
      aiHints.ai_protocol = IPPROTO_UDP;
      error = ::getaddrinfo(host, 0, &aiHints, &aiList);
      if (error || !aiList) {
         if ( debug_ ) 
            cout << "UdpLink::open -> Failed to open UDP host " << host << ":" << port << endl;
         udpFd_[x]   = -1;
      }
      else {
         addr = (const sockaddr_in*)(aiList->ai_addr);

         // Setup Remote Address
         memset(&(udpAddr_[x]),0,sizeof(struct sockaddr_in));
         ((struct sockaddr_in *)(&(udpAddr_[x])))->sin_family=AF_INET;
         ((struct sockaddr_in *)(&(udpAddr_[x])))->sin_addr.s_addr=addr->sin_addr.s_addr;
         ((struct sockaddr_in *)(&(udpAddr_[x])))->sin_port=htons(port);

         // Debug
         if ( debug_ ) 
            cout << "UdpLink::open -> Opened UDP device " << host << ":" << port << ", Fd=" << dec << udpFd_[x]
                 << ", Addr=0x" << hex << ((struct sockaddr_in *)(&(udpAddr_[x])))->sin_addr.s_addr << endl;
      }
   }

   // Start threads
   CommLink::open();
}

// Stop threads and close link
void UdpLink::close () {
   if ( udpFd_ != NULL ) {
      CommLink::close();
      usleep(100);
      for(uint x=0; x < udpCount_; x++) { 
         if ( udpFd_[x] >= 0 ) ::close(udpFd_[x]);
         udpFd_[x] = -1;
      }
      free(udpFd_);
   }
   if ( udpAddr_ != NULL ) free(udpAddr_);
   udpAddr_  = NULL;
   udpFd_    = NULL;
   udpCount_ = 0;
}

