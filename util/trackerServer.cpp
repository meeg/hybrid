//-----------------------------------------------------------------------------
// File          : guiServer.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Server application for GUI
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//----------------------------------------------------------------------------
#include <UdpLink.h>
#include <PgpLink.h>
#include <Tracker.h>
#include <ControlServer.h>
#include <Device.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <signal.h>
using namespace std;

// Run flag for sig catch
bool stop;

// Function to catch cntrl-c
void sigTerm (int) {
   stop = true;
   cout << "Stopping gui server" << endl;
}

int main (int argc, char **argv) {
   PgpLink       pgpLink; 
   UdpLink       udpLink; 
   CommLink      commLink; 
   Tracker       *tracker;
   ControlServer cntrlServer;
   string        xmlTest;

   // Catch signals
   signal (SIGINT,&sigTerm);
   cout << "Starting gui server" << endl;

   try {

      if ( argc > 1 ) {

         // Create and setup PGP link
         tracker = new Tracker(&commLink);
         commLink.setDebug(true);
         commLink.open();
         cout << "Using debug interface" << endl;

      } else {

         // Create and setup PGP link
         //tracker = new Tracker(&pgpLink);
         //pgpLink.setMaxRxTx(500000);
         //pgpLink.setDebug(true);
         //pgpLink.open("/dev/pgpcard0");
         //usleep(100);
         //cout << "Using PGP interface" << endl;

         tracker = new Tracker(&udpLink);
         udpLink.setMaxRxTx(500000);
         udpLink.setDebug(true);
         udpLink.open(8192,1,"192.168.0.16");
         udpLink.openDataNet("127.0.0.1",8099);
         usleep(100);
         cout << "Using UDP interface" << endl;
      }

      // Setup control server
      //cntrlServer.setDebug(true);
      cntrlServer.startListen(8092);
      cntrlServer.setSystem(tracker);

      // Start listen
      while ( ! stop ) cntrlServer.receive(100);
      cntrlServer.stopListen();
      cout << "Stopped gui server" << endl;

   } catch ( string error ) {
      cout << "Caught Error: " << endl;
      cout << error;
      cntrlServer.stopListen();
   }
}

