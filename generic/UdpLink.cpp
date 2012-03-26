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

// Receive Thread
void UdpLink::rxHandler() {
   uint           rxMask;
   uint           rxIdx;
   uint           *rxBuff[udpCount_];
   uint           rxSize[udpCount_];
   int            maxFd;
   struct timeval timeout;
   int            ret;
   fd_set         fds;
   unsigned char  qbuffer[2];
   struct msghdr  msgHdr;
   struct iovec   vecHdr[2];
   bool           rSof;
   bool           rEof;
   uint           rVc;
   uint           udpcnt;
   uint           x;
   Data           *data;
   
   // Init buffers
   for ( rxIdx=0; rxIdx < udpCount_; rxIdx++ ) {
      rxBuff[rxIdx] = (uint *) malloc(sizeof(uint)*maxRxTx_);
      rxSize[rxIdx] = 0;
   }

   // Init scatter/gather structure
   msgHdr.msg_namelen    = sizeof(struct sockaddr_in);   
   msgHdr.msg_iov        = vecHdr;
   vecHdr[0].iov_base    = qbuffer;
   vecHdr[0].iov_len     = 2;
   msgHdr.msg_iovlen     = 2;
   msgHdr.msg_control    = NULL;
   msgHdr.msg_controllen = 0;
   msgHdr.msg_flags      = 0;

   // While enabled
   while ( runEnable_ ) {

      // Init fds
      FD_ZERO(&fds);
      maxFd = 0;

      // Process each dest
      for ( rxIdx=0; rxIdx < udpCount_; rxIdx++ ) {
         FD_SET(udpFd_[rxIdx],&fds);
         if ( udpFd_[rxIdx] > maxFd ) maxFd = udpFd_[rxIdx];
      }

      // Setup timeout
      timeout.tv_sec  = 0;
      timeout.tv_usec = 500;

      // Select
      if ( select(maxFd+1, &fds, NULL, NULL, &timeout) <= 0 ) continue;

      // Which FD had data
      for ( rxIdx=0; rxIdx < udpCount_; rxIdx++ ) {
         if ( FD_ISSET(udpFd_[rxIdx],&fds) ) {

            // Setup scatter/gather header
            msgHdr.msg_name    = &(udpAddr_[rxIdx]);
            vecHdr[1].iov_base = &(rxBuff[rxIdx][rxSize[rxIdx]]);
            vecHdr[1].iov_len  = (maxRxTx_-rxSize[rxIdx]) * 4;

            // Attempt receive
            ret = recvmsg(udpFd_[rxIdx],&msgHdr,0);

            // No data
            if ( ret <= 0 ) continue;

            // Extract header
            rSof    = (qbuffer[0] >> 7) & 0x1;
            rEof    = (qbuffer[0] >> 6) & 0x1;
            rVc     = (qbuffer[0] >> 4) & 0x3;
            //udpcnt  = (qbuffer[0] << 8) & 0xF00;
            udpcnt  =  qbuffer[1] & 0xFF;
            udpcnt -= 1;

            // Bad sof
            if (( rSof && rxSize[rxIdx] != 0 ) || ( !rSof && rxSize[rxIdx] == 0 )) {
               cout << "UdpLink::rxHandler -> Bad sof in header."
                    << " Sof=" << dec << rSof << " count=" << dec << rxSize[rxIdx] << endl;
               errorCount_++;
               rxSize[rxIdx] = 0;
               continue;
            }

            // Bad size
            if ( ret < 16 || (udpcnt % 2) != 0 )  {
               cout << "UdpLink::rxHandler -> Bad length in header. udpcnt=" 
                    << dec << udpcnt << ", ret=" << dec << ret
                    << ", iov_len=" << dec << vecHdr[1].iov_len << endl;
               errorCount_++;
               rxSize[rxIdx] = 0;
               continue;
            }
            rxSize[rxIdx] += (((uint)ret-2)/4);

            // End of frame
            if ( rEof ) {

               // Check for data packet
               rxMask = 1 << rVc;
               if ( (dataMask_ & rxMask) != 0 ) {
 
                  // Reformat data
                  if ( dataOrderFix_ ) {          
                     for ( x=0; x < rxSize[rxIdx]; x++ ) rxBuff[rxIdx][x] = ntohl(rxBuff[rxIdx][x]);
                  }
                  data = new Data(rxBuff[rxIdx],rxSize[rxIdx]);
                  dataQueue_.push(data);
               }

               // Reformat header for register rx
               else {
                  for ( x=0; x < rxSize[rxIdx]; x++ ) rxBuff[rxIdx][x] = ntohl(rxBuff[rxIdx][x]);

                  // Data matches outstanding register request
                  if ( rxIdx == regReqDest_ && memcmp(rxBuff[rxIdx],regBuff_,8) == 0 && 
                       (uint)(rxSize[rxIdx]-3) == regReqEntry_->size()) {
                     if ( ! regReqWrite_ ) {
                        if ( rxBuff[rxIdx][rxSize[rxIdx]-1] == 0 ) 
                           memcpy(regReqEntry_->data(),&(rxBuff[rxIdx][2]),(regReqEntry_->size()*4));
                     }
                     regReqEntry_->setStatus(rxBuff[rxIdx][rxSize[rxIdx]-1]);
                     regRespCnt_++;
                  }

                  // Unexpected frame
                  else {
                     unexpCount_++;
                     if ( debug_ ) {
                        cout << "UdpLink::rxHandler -> Unuexpected frame received"
                             << " Comp=" << dec << (memcmp(rxBuff[rxIdx],regBuff_,8))
                             << " Word0_Exp=0x" << hex << regBuff_[0]
                             << " Word0_Got=0x" << hex << rxBuff[rxIdx][0]
                             << " Word1_Exp=0x" << hex << regBuff_[1]
                             << " Word1_Got=0x" << hex << rxBuff[rxIdx][1]
                             << " ExpSize=" << dec << regReqEntry_->size()
                             << " GotSize=" << dec << (rxSize[rxIdx]-3) 
                             << " DataMaskRx=0x" << hex << rxMask
                             << " DataMask=0x" << hex << dataMask_ << endl;
                     }
                  }
               }
               rxSize[rxIdx] = 0;
            }
         }
      }
   }

   for ( rxIdx=0; rxIdx < udpCount_; rxIdx++ ) free(rxBuff[rxIdx]);
}

// Transmit thread
void UdpLink::ioHandler() {
   uint           cmdBuff[4];
   uint           runBuff[4];
   uint           lastReqCnt;
   uint           lastCmdCnt;
   uint           lastRunCnt;
   uint           udpVc;
   uint           udpDest;
   uint           udpSize;
   uint           *udpBuff;
   unsigned char  byteData[9000];
   uint           usize;
   uint           *uintBuff;
   uint           x;
   
   // Setup
   lastReqCnt = regReqCnt_;
   lastCmdCnt = cmdReqCnt_;
   lastRunCnt = runReqCnt_;

   // Init register buffer
   regBuff_ = (uint *) malloc(sizeof(uint)*maxRxTx_);

   // While enabled
   while ( runEnable_ ) {
      udpSize = 0;

      // Run Command TX is pending
      if ( lastRunCnt != runReqCnt_ ) {

         // Setup tx buffer
         runBuff[0]  = 0;
         runBuff[1]  = runReqEntry_->opCode() & 0xFF;
         runBuff[2]  = 0;
         runBuff[3]  = 0;
 
         // Setup transmit
         udpDest = runReqDest_;
         udpSize = 4;
         udpBuff = runBuff;
         udpVc   = (runReqEntry_->opCode()>>8)  & 0xF;
          
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
            regBuff_[2]  = (regReqEntry_->size()-1) & 0x3FF;
            regBuff_[2] |= (regBuff_[2] << 16);
            regBuff_[3]  = 0;
         }
 
         // Setup transmit
         udpDest = regReqDest_;
         udpSize = (regReqWrite_)?regReqEntry_->size()+3:4;
         udpBuff = regBuff_;
         udpVc   = (regReqEntry_->address()>>24) & 0xF;

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
         udpDest = cmdReqDest_;
         udpSize = 4;
         udpBuff = cmdBuff;
         udpVc   = (cmdReqEntry_->opCode()>>8) & 0xF;

         // Match request count
         lastCmdCnt = cmdReqCnt_;
         cmdRespCnt_++;
      }

      // Transmit needed and valid
      if ( udpSize != 0 && (udpSize * 4) <= 8000 && udpDest < udpCount_ && udpFd_[udpDest] > 0 ) {
         usize = udpSize * 2;

         // Setup header
         byteData[0]  = 0xC0; // SOF & EOF
         byteData[0] += (udpVc << 4) & 0x30;
         byteData[0] += (usize >> 8) & 0xF;
         byteData[1]  = usize & 0xFF;

         // Setup integer pointer
         uintBuff = (uint *)(&(byteData[2]));

         // Copy payload integer by integer and convert
         for ( x=0; x < udpSize; x++ ) uintBuff[x] = htonl(udpBuff[x]);

         // Send
         sendto(udpFd_[udpDest],byteData,(udpSize*4)+2,0,(struct sockaddr *)(&(udpAddr_[udpDest])),sizeof(struct sockaddr_in));
      } 
      usleep(10);
   }

   free(regBuff_);
}

// Constructor
UdpLink::UdpLink ( ) : CommLink() {
   udpFd_        = NULL;
   udpAddr_      = NULL;
   udpCount_     = 0;
   dataOrderFix_ = true;
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
   uint                x;
   const char *        host;
   unsigned int        rwin;
   socklen_t           rwin_size=4;

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
      size = 5000000;
      setsockopt(udpFd_[x], SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
      getsockopt(udpFd_[x], SOL_SOCKET, SO_RCVBUF, &rwin, &rwin_size);
      if(size > rwin) {
         cout << "--------------------------------" << endl;
         cout << "Error setting rx buffer size."    << endl;
         cout << "Wanted " << dec << size << " Got " << dec << rwin << endl;
         cout << "--------------------------------" << endl;
      }
      cout << "UdpLink::open -> Rx buffer size=" << dec << rwin << endl;

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

// Set data order fix flag
void UdpLink::setDataOrderFix (bool enable) {
   dataOrderFix_ = enable;
}

