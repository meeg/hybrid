//-----------------------------------------------------------------------------
// File          : UdpServer.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP server for data connections.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <string>
#include <QDomDocument>
#include <QMessageBox>
#include "UdpServer.h"
using namespace std;

// Constructor
UdpServer::UdpServer (int port) {

   // Network setup
   udpSocket_ = new QUdpSocket(this);
   udpSocket_->bind(port);
   connect(udpSocket_, SIGNAL(readyRead()), this, SLOT(sockReady()));
   debug_     = false;
   lastSize_  = 0;
   dataCount_ = 0;
}

// Delete
UdpServer::~UdpServer ( ) { 
   delete udpSocket_;
}

void UdpServer::setDebug(bool debug) {
   debug_ = debug;
}

void UdpServer::sockReady() {
   QByteArray datagram;
   QHostAddress sender;
   quint16 senderPort;

   while (udpSocket_->hasPendingDatagrams()) {

      datagram.resize(udpSocket_->pendingDatagramSize());
      udpSocket_->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

      // Size = 4 means this is the size marker
      if ( datagram.size() == 4 ) {
         lastSize_ = *((uint *)datagram.constData());
      }
      else {

         // Data received
         if ( (lastSize_ & 0xF0000000) == 0 ) {

            // Check size
            if ( (lastSize_*4) != (uint)datagram.size() ) {
               cout << "Size mismatch Got=" << dec << datagram.size() 
                    << " Exp=" << dec << (lastSize_*4) << endl;
            }
            else {
               rxData ((lastSize_&0x0FFFFFFF), (uint *)datagram.constData());
               dataCount_++;
            }
         }

         // Config or status
         else if ( ((lastSize_ & 0xF0000000) == 0x10000000 ) ||
                   ((lastSize_ & 0xF0000000) == 0x20000000 ) ) {

            // Check size
            if ( (lastSize_&0x0FFFFFFF) != (uint)datagram.size() ) {
               cout << "Size mismatch Got=" << dec << datagram.size() 
                    << " Exp=" << dec << (lastSize_&0x0FFFFFFF) << endl;
            }
            else {

               // Parse
               QDomDocument doc("temp");
               doc.setContent(datagram);

               // Get top level
               QDomElement elem = doc.documentElement();
               QDomNode node;

               // Process first child element
               node = elem.firstChild();

               // Config
               if ( (lastSize_ & 0xF0000000) == 0x10000000 ) {
                  cout << "Got Config. Data Count=" << dec << dataCount_ << endl;
                  xmlConfig(node);
               }

               // Status               
               else if ( (lastSize_ & 0xF0000000) == 0x20000000 ) {
                  cout << "Got Status. Data Count=" << dec << dataCount_ << endl;
                  xmlStatus(node);
               }
            }
         }
         lastSize_ = 0;
      }
   }
}

