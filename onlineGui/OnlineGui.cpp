//-----------------------------------------------------------------------------
// File          : OnlineGui.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 03/22/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Main program
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 03/22/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <QApplication>
#include <QErrorMessage>
#include <QObject>
#include "UdpServer.h"
#include "MainWindow.h"
using namespace std;

// Main Function
int main ( int argc, char **argv ) {
   int     port;

   // No args, use default
   if ( argc < 2 ) port = 8092;

   // Proper args
   else if ( argc == 2 ) port = atoi(argv[1]);

   // Show usage
   else {
      cout << "Usage: onlineGUi [port]" << endl;
      exit(-1);
   }

   // Start application
   QApplication a( argc, argv );

   UdpServer udpServer(8099);
   udpServer.setDebug(true);

   MainWindow mainWin;
   mainWin.show();

   // Udp signals
   QObject::connect(&udpServer,SIGNAL(xmlStatus(QDomNode)),&mainWin,SLOT(xmlStatus(QDomNode)));
   QObject::connect(&udpServer,SIGNAL(xmlConfig(QDomNode)),&mainWin,SLOT(xmlConfig(QDomNode)));
   QObject::connect(&udpServer,SIGNAL(rxData(uint,uint *)),&mainWin,SLOT(rxData(uint, uint*)));

   // Exit on window close
   QObject::connect(&a,SIGNAL(lastWindowClosed()), &a, SLOT(quit())); 

   // Run application
   return(a.exec());
}

