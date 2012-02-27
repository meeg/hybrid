//-----------------------------------------------------------------------------
// File          : codaGui.cpp
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
#include <TrackerLink.h>
#include <TrackerFull.h>
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
   //UdpLink       udpLink; 
   TrackerLink   udpLink; 
   CommLink      commLink; 
   TrackerFull   *tracker;
   ControlServer cntrlServer;
   string        xmlTest;
   int           pid;

   // Catch signals
   signal (SIGINT,&sigTerm);
   cout << "Starting gui server" << endl;

   try {

      tracker = new TrackerFull(&udpLink);
      udpLink.setMaxRxTx(500000);
      udpLink.setDebug(true);
      udpLink.open(8192,8,"192.168.1.23",
                          "192.168.1.16",
                          "192.168.1.17",
                          "192.168.1.18",
                          "192.168.1.19",
                          "192.168.1.20",
                          "192.168.1.21",
                          "192.168.1.22");
      usleep(100);
      cout << "Using UDP interface" << endl;

      // Setup control server
      //cntrlServer.setDebug(true);
      cntrlServer.startListen(8093);
      cntrlServer.setSystem(tracker);

      // Fork and start gui
      stop = false;
      switch (pid = fork()) {

         // Error
         case -1:
            cout << "Error occured in fork!" << endl;
            return(1);
            break;

         // Child
         case 0:
            cout << "Starting GUI" << endl;
            system("cntrlGui localhost 8093");
            cout << "Gui stopped" << endl;
            kill(getppid(),SIGINT);
            break;

         // Server
         default:
            while ( ! stop ) cntrlServer.receive(100);
            kill(pid,SIGINT);
            sleep(1);
            cntrlServer.stopListen();
            cout << "Stopped gui server" << endl;
            break;
      }

   } catch ( string error ) {
      cout << "Caught Error: " << endl;
      cout << error;
      cntrlServer.stopListen();
   }
}

