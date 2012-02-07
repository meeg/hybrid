//-----------------------------------------------------------------------------
// File          : XmlClient.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// XML client for server connections
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __XML_CLIENT_H__
#define __XML_CLIENT_H__

#include <QTcpSocket>
#include <QObject>
#include <QTimer>
#include <QDomDocument>
using namespace std;

class XmlClient : public QObject {
   
   Q_OBJECT

      // Host status
      bool hostOpen_;

      // Network
      QTcpSocket  *tcpSocket_;
      QTextStream *tcpStream_;

      // Receive buffer
      QString   xmlBuffer_;

      // Debug
      bool debug_;

   public:

      // Creation Class
      XmlClient ( );

      // Delete
      ~XmlClient ( );

      // Set debug
      void setDebug(bool debug);

   public slots:

      void openServer (QString host, int port);
      void closeServer();
      void sockConnected();
      void sockDisconnected();
      void sockError(QAbstractSocket::SocketError socketError);
      void sockReady();

      // Send commands
      void sendCommand(QString cmd);

      // Send config
      void sendConfig(QString cfg);

      // Send config & command
      void sendConfigCommand(QString cfg, QString cmd);

   signals:

      // XML Message received
      void xmlMessage (QDomNode node);

      // Lost connection
      void lostConnection ();

      // Error found
      void foundError ();
};

#endif
