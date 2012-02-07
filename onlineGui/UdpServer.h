//-----------------------------------------------------------------------------
// File          : UdpServer.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP server for data
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__

#include <QUdpSocket>
#include <QObject>
#include <QTimer>
#include <QDomDocument>
using namespace std;

class UdpServer : public QObject {
   
   Q_OBJECT

      // Network
      QUdpSocket  *udpSocket_;

      // Debug
      bool debug_;

      // Last size value received
      uint lastSize_;

      // Data counter
      uint dataCount_;

   public:

      // Creation Class
      UdpServer (int port);

      // Delete
      ~UdpServer ( );

      // Set debug
      void setDebug(bool debug);

   public slots:

      void sockReady();

   signals:

      // XML Status received
      void xmlStatus (QDomNode node);

      // XML Config received
      void xmlConfig (QDomNode node);

      // Data
      void rxData (uint size, uint *data);
};

#endif
