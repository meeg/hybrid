//-----------------------------------------------------------------------------
// File          : TrackerTis.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon TrackerTis
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
#include <TrackerTis.h>
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
TrackerTis::TrackerTis (CommLink *commLink) : System("TrackerTis",commLink) {

   // Description
   desc_     = "Tracker Tis Control";
   defaults_ = "tis_defaults.xml";

   // Add sub-devices
   addDevice(new TisFpga(0, 0,this));

   // Commands
   addCommand(new Command("TisSWTrig"));
   commands_["TisSWTrig"]->setDescription("Start TIS trigger.");
}

// Deconstructor
TrackerTis::~TrackerTis ( ) { }

// Method to process a command
void TrackerTis::command ( string name, string arg ) {

   if ( name == "TisSWTrig" ) device("tisFpga",0)->command("TisSWTrig","");

   else System::command(name,arg);
}

// Method to set run state
void TrackerTis::setRunState ( string state ) {
   if ( !swRunning_ ) device("tisFpga",0)->setRunCommand("TisSWTrig");
   System::setRunState(state);
}

//! Return local state machine, specific to each implementation
string TrackerTis::localState() {
   string loc = "";

   loc = "System Ready To Take Data.\n";      

   return(loc);
}

//! Method to perform soft reset
void TrackerTis::softReset ( ) {
   System::softReset();
   //device("cntrlFpga",0)->command("Apv25Reset","");
   //sleep(5);
}

//! Method to perform hard reset
void TrackerTis::hardReset ( ) {
   System::hardReset();

   //bool gotVer = false;
   //uint count = 0;
/*
   device("cntrlFpga",0)->command("MasterReset","");

   do {
      sleep(1);
      try { 
         gotVer = true;
         device("cntrlFpga",0)->readSingle("Version");
      } catch ( string err ) { 
         if ( count > 5 ) {
            gotVer = true;
            throw(string("TrackerTis::hardReset -> Error contacting concentrator"));
         }
         else {
            count++;
            gotVer = false;
         }
      }
   } while ( !gotVer );
   device("cntrlFpga",0)->command("Apv25HardReset","");
*/
}

