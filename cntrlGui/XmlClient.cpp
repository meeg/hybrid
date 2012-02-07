//-----------------------------------------------------------------------------
// File          : XmlClient.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// XML client for server connections.
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
#include "XmlClient.h"
using namespace std;

// Constructor
XmlClient::XmlClient ( ) {

   // Network setup
   tcpSocket_ = new QTcpSocket(this);
   connect(tcpSocket_, SIGNAL(readyRead()), this, SLOT(sockReady()));
   connect(tcpSocket_, SIGNAL(connected()), this, SLOT(sockConnected()));
   connect(tcpSocket_, SIGNAL(disconnected()), this, SLOT(sockDisconnected()));
   connect(tcpSocket_, SIGNAL(error(QAbstractSocket::SocketError)), 
           this,       SLOT(sockError(QAbstractSocket::SocketError)));
   tcpStream_ = new QTextStream(tcpSocket_);
   debug_     = false;
   xmlBuffer_ = "";
}


// Delete
XmlClient::~XmlClient ( ) { 
   delete tcpStream_;
   delete tcpSocket_;
}

void XmlClient::setDebug(bool debug) {
   debug_ = debug;
}

void XmlClient::openServer (QString host, int port) {
   if ( debug_ ) cout << "XmlClient::openServer -> Connecting to server " << qPrintable(host) << ":" << port << endl;
   tcpSocket_->connectToHost(host,port);
}

void XmlClient::closeServer() {
   if ( debug_ ) cout << "XmlClient::closeServer -> Closing server" << endl;
   tcpSocket_->disconnectFromHost();
}

// Network callbacks
void XmlClient::sockConnected() { }

void XmlClient::sockDisconnected() {
   if ( debug_ ) cout << "XmlClient::sockDisconnected -> Socket Disconnected." << endl;
   lostConnection();
}

void XmlClient::sockReady() {

   // Append buffer
   xmlBuffer_.append(tcpStream_->readAll());

   // Look for end character
   if ( xmlBuffer_.contains("\f") ) {

      // Parse
      QDomDocument doc("temp");
      doc.setContent(xmlBuffer_);
      xmlBuffer_ = "";

      // Get top level
      QDomElement elem = doc.documentElement();
      QDomNode node;

      // Process first child element
      node = elem.firstChild();
      
      // Process each sub sibling
      while ( ! node.isNull() ) {

         // Node is an error
         if ( node.isElement() && node.nodeName() == "error" ) {
            if ( debug_ ) 
               cout << "XmlClient::findErrors -> Found Error: " 
                    << qPrintable(node.firstChild().nodeValue()) << endl;

            // Signal error
            foundError();

            // Messagebox
            QMessageBox::warning(NULL,QString("Error!"),node.firstChild().nodeValue(),QMessageBox::Ok);
         }

         // Distribute node
         else xmlMessage(node);

         node = node.nextSibling();
      }
   }
}

void XmlClient::sockError(QAbstractSocket::SocketError ) {
   if ( debug_ ) cout << "XmlClient::sockError -> Socket error detected." << endl;
   lostConnection();
}

// Send commands
void XmlClient::sendCommand ( QString cmd ) {
   *tcpStream_ << QString("<system><command>");
   *tcpStream_ << cmd;
   *tcpStream_ << QString("</command></system>\n\f");
   tcpStream_->flush();
}

// Send config
void XmlClient::sendConfig ( QString cfg ) {
   *tcpStream_ << QString("<system><config>");
   *tcpStream_ << cfg;
   *tcpStream_ << QString("</config></system>\n\f");
   tcpStream_->flush();
}

// Send config & command
void XmlClient::sendConfigCommand(QString cfg, QString cmd) {
   *tcpStream_ << QString("<system><config>");
   *tcpStream_ << cfg;
   *tcpStream_ << QString("</config><command>");
   *tcpStream_ << cmd;
   *tcpStream_ << QString("</command></system>\n\f");
   tcpStream_->flush();
}

