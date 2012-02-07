//-----------------------------------------------------------------------------
// File          : CntrlGui.cpp
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
#include "XmlClient.h"
#include "MainWindow.h"
#include "SystemWindow.h"
#include "CommandWindow.h"
#include "VariableWindow.h"
#include "ScriptWindow.h"
using namespace std;

// Main Function
int main ( int argc, char **argv ) {
   QString host;
   int     port;

   // No args, use default
   if ( argc < 2 ) {
      host = "localhost";
      port = 8092;
   }

   // Proper args
   else if ( argc == 3 ) {
      host = argv[1];
      port = atoi(argv[2]);
   }

   // Show usage
   else {
      cout << "Usage: cntrlGUi [host port]" << endl;
      exit(-1);
   }

   // Start application
   QApplication a( argc, argv );

   XmlClient xmlClient;
   xmlClient.setDebug(true);

   MainWindow mainWin;
   mainWin.show();

   // System signals
   QObject::connect(mainWin.systemWindow,SIGNAL(sendCommand(QString)),&xmlClient,SLOT(sendCommand(QString)));
   QObject::connect(mainWin.systemWindow,SIGNAL(sendConfigCommand(QString,QString)),&xmlClient,SLOT(sendConfigCommand(QString,QString)));
   QObject::connect(mainWin.systemWindow,SIGNAL(stopScript()),mainWin.scriptWindow,SLOT(stopScript()));

   // Command signals
   QObject::connect(mainWin.commandWindow,SIGNAL(sendCommand(QString)),&xmlClient,SLOT(sendCommand(QString)));

   // Status signals
   QObject::connect(mainWin.statusWindow,SIGNAL(sendCommand(QString)),&xmlClient,SLOT(sendCommand(QString)));

   // Config signals
   QObject::connect(mainWin.configWindow,SIGNAL(sendCommand(QString)),&xmlClient,SLOT(sendCommand(QString)));
   QObject::connect(mainWin.configWindow,SIGNAL(sendConfig(QString)),&xmlClient,SLOT(sendConfig(QString)));

   // Script signals
   QObject::connect(mainWin.scriptWindow,SIGNAL(sendCommand(QString)),&xmlClient,SLOT(sendCommand(QString)));
   QObject::connect(mainWin.scriptWindow,SIGNAL(sendConfig(QString)),&xmlClient,SLOT(sendConfig(QString)));
   QObject::connect(mainWin.scriptWindow,SIGNAL(sendConfigCommand(QString,QString)),&xmlClient,SLOT(sendConfigCommand(QString,QString)));
   QObject::connect(mainWin.scriptWindow,SIGNAL(stateMessage(QString)),mainWin.systemWindow,SLOT(scriptMessage(QString)));

   // XML signals
   QObject::connect(&xmlClient,SIGNAL(xmlMessage(QDomNode)),mainWin.systemWindow,SLOT(xmlMessage(QDomNode)));
   QObject::connect(&xmlClient,SIGNAL(xmlMessage(QDomNode)),mainWin.commandWindow,SLOT(xmlMessage(QDomNode)));
   QObject::connect(&xmlClient,SIGNAL(xmlMessage(QDomNode)),mainWin.statusWindow,SLOT(xmlMessage(QDomNode)));
   QObject::connect(&xmlClient,SIGNAL(xmlMessage(QDomNode)),mainWin.configWindow,SLOT(xmlMessage(QDomNode)));
   QObject::connect(&xmlClient,SIGNAL(xmlMessage(QDomNode)),mainWin.scriptWindow,SLOT(xmlMessage(QDomNode)));
   QObject::connect(&xmlClient,SIGNAL(xmlMessage(QDomNode)),&mainWin,SLOT(xmlMessage(QDomNode)));
   QObject::connect(&xmlClient,SIGNAL(foundError()),mainWin.scriptWindow,SLOT(stopScript()));

   // Exit on lost connection
   QObject::connect(&xmlClient,SIGNAL(lostConnection()),&a,SLOT(closeAllWindows()));
   QObject::connect(&a,SIGNAL(lastWindowClosed()), &a, SLOT(quit())); 

   // Open host
   cout << "Connecting to host " << qPrintable(host) << ", Port " << port << endl;
   xmlClient.openServer(host,port);

   // Run application
   return(a.exec());
}

