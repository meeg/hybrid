//-----------------------------------------------------------------------------
// File          : Tracker.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// Control FPGA container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <Tracker.h>
#include <CntrlFpga.h>
#include <TisFpga.h>
#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <CommLink.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
Tracker::Tracker (CommLink *commLink) : System("Tracker",commLink) {

   // Description
   desc_ = "Tracker Control";
   
   // Data mask, lane 0, vc 0
   commLink_->setDataMask(0x11);

   // Add sub-devices
   addDevice(new CntrlFpga(0, 0,this));
   //addDevice(new TisFpga(1, 0,this));
}

// Deconstructor
Tracker::~Tracker ( ) { }

// Method to process a command
void Tracker::command ( string name, string arg ) {
   System::command(name,arg);
}

// Method to set run state
void Tracker::setRunState ( string state ) {
   if ( !swRunning_ ) device("cntrlFpga",0)->setRunCommand("ApvSWTrig");
   //if ( !swRunning_ ) device("tisFpga",0)->setRunCommand("TisSWTrig");
   System::setRunState(state);
}

//! Return local state, specific to each implementation
string Tracker::localState() {
   stringstream loc;
   uint         apv;
   uint         bit;
   uint         hyb;
   uint         fpga;
   uint         apvCnt;
   uint         syncCnt;
   uint         reg;
   bool         fifoErr;
   bool         latErr;
   uint         fpgaMask;
   stringstream stat;

   apvCnt    = 0;
   syncCnt   = 0;
   fifoErr   = false;
   latErr    = false;
   fpgaMask  = 0x80; // TI Always Enabled
   loc.str("");

   // Check hybrid status
   for ( fpga = 0; fpga < 1; fpga++ ) { 
      if ( device("cntrlFpga",fpga)->get("enabled") == "True" ) {
         fpgaMask |= (1 << fpga);
      
         // Get sync register
         reg = device("cntrlFpga",fpga)->getInt("ApvSyncDetect");

         for ( hyb = 0; hyb < 1; hyb++ ) { 
            if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->get("enabled") == "True" ) {

               // Check APV status registers
               for ( apv = 0; apv < 5; apv++ ) {
                  if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->get("enabled") == "True" ) {
                     if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->getInt("FifoError") != 0 ) {
                        fifoErr = true;
                     }
                     if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->getInt("LatencyError") != 0 ) {
                        latErr = true;
                     }

                     apvCnt++;
                  }

                  // Count synced apvs
                  bit = (hyb * 5) + apv;
                  if (((reg >> bit) & 0x1 ) != 0) syncCnt++;
               }
            }
         }
      }
   }

   // Update status message
   if ( fifoErr ) loc << "APV FIFO Error.\n";
   if ( latErr  ) loc << "APV Latency Error.\n";
   loc << dec << syncCnt << " Out Of " << dec << apvCnt << " Apvs Are Synced!\n";
   if ( (!fifoErr) && (!latErr) ) loc << "System Ready To Take Data.\n";      
   else loc << "Soft Reset Required!\n";

   return(loc.str());
}

//! Method to perform soft reset
void Tracker::softReset ( ) {

   System::softReset();

   device("cntrlFpga",0)->command("ApvClkRst","");
   sleep(1);
   device("cntrlFpga",0)->command("Apv25Reset","");
   sleep(1);
   device("cntrlFpga",0)->command("Apv25Reset","");
   sleep(1);
}

//! Method to perform hard reset
void Tracker::hardReset ( ) {
   bool gotVer = false;
   uint count = 0;

   device("cntrlFpga",0)->device("hybrid",0)->set("enabled","False");
   device("cntrlFpga",0)->device("hybrid",1)->set("enabled","False");
   device("cntrlFpga",0)->device("hybrid",2)->set("enabled","False");

   System::hardReset();

   do {
      sleep(1);
      try { 
         gotVer = true;
         device("cntrlFpga",0)->readSingle("Version");
      } catch ( string err ) { 
         if ( count > 5 ) {
            gotVer = true;
            throw(string("TrackerFull::hardReset -> Error reading version regsiter"));
         }
         else {
            count++;
            gotVer = false;
         }
      }
   } while ( !gotVer );
   device("cntrlFpga",0)->command("Apv25HardReset","");
   usleep(100);
}

