//-----------------------------------------------------------------------------
// File          : ApvMux.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// APV25, I2C Register Container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <ApvMux.h>
#include <Register.h>
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
ApvMux::ApvMux ( uint destination, uint baseAddress, Device *parent ) : 
                        Device(destination,baseAddress,"apvMux",0,parent) {


   // Description
   desc_ = "APV MUX Object.";

   // Create Registers: name, address
   addRegister(new Register("Resistor", baseAddress_ + 0x03));

   // Create variables
   addVariable(new Variable("Resistor", Variable::Configuration));
   variables_["Resistor"]->setDescription("Resistor Data.");

   variables_["Enabled"]->set("False");

}

// Deconstructor
ApvMux::~ApvMux ( ) { }

// Method to read configuration registers and update variables
void ApvMux::readConfig (  ) {
   REGISTER_LOCK

   // Read Configuration
   readRegister(registers_["Resistor"]);
   variables_["Resistor"]->setInt(registers_["Resistor"]->get());
   REGISTER_UNLOCK
}

// Method to write configuration registers
void ApvMux::writeConfig ( bool force ) {
   REGISTER_LOCK

   registers_["Resistor"]->set(variables_["Resistor"]->getInt());
   writeRegister(registers_["Resistor"],force);
   REGISTER_UNLOCK
}

// Verify hardware state of configuration
void ApvMux::verifyConfig ( ) {
   REGISTER_LOCK
   verifyRegister(registers_["Resistor"]);
   REGISTER_UNLOCK
}

