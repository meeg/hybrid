//-----------------------------------------------------------------------------
// File          : Ad9510.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/20/2012
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// AD9510 ADC
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/20/2012: created
//-----------------------------------------------------------------------------
#include <Ad9510.h>
#include <Register.h>
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
Ad9510::Ad9510 ( uint destination, uint baseAddress, uint index, Device *parent ) : 
                        Device(destination,baseAddress,"ad9510",index,parent) {

   // Description
   desc_ = "AD9510 PLL object.";

   // Create Registers: name, address
   addRegister(new Register("ChipPortConfig",  baseAddress_ + 0x00));

   addRegister(new Register("Divider5RegA",    baseAddress_ + 0x52));
   addRegister(new Register("Divider5RegB",    baseAddress_ + 0x53));
   addRegister(new Register("Divider6RegA",    baseAddress_ + 0x54));
   addRegister(new Register("Divider6RegB",    baseAddress_ + 0x55));

   addRegister(new Register("UpdateRegister",  baseAddress_ + 0x5A));

   // Variables
   addVariable(new Variable("Divider5LowCycles", Variable::Configuration));
   variables_["Divider5LowCycles"]->setDescription("Divider 5 Low Cycles.");
   variables_["Divider5LowCycles"]->setRange(0,15);

   addVariable(new Variable("Divider5HighCycles", Variable::Configuration));
   variables_["Divider5HighCycles"]->setDescription("Divider 5 High Cycles.");
   variables_["Divider5HighCycles"]->setRange(0,15);

   addVariable(new Variable("Divider5Bypass", Variable::Configuration));
   variables_["Divider5Bypass"]->setDescription("Divider 5 Bypass.");
   variables_["Divider5Bypass"]->setTrueFalse();

   addVariable(new Variable("Divider5NoSync", Variable::Configuration));
   variables_["Divider5NoSync"]->setDescription("Divider 5 No Sync.");
   variables_["Divider5NoSync"]->setTrueFalse();

   addVariable(new Variable("Divider5Force", Variable::Configuration));
   variables_["Divider5Force"]->setDescription("Divider 5 Force.");
   variables_["Divider5Force"]->setTrueFalse();

   addVariable(new Variable("Divider5StartHigh", Variable::Configuration));
   variables_["Divider5StartHigh"]->setDescription("Divider 5 Start High.");
   variables_["Divider5StartHigh"]->setTrueFalse();

   addVariable(new Variable("Divider5Phase", Variable::Configuration));
   variables_["Divider5Phase"]->setDescription("Divider 5 Phase Offset.");
   variables_["Divider5Phase"]->setRange(0,15);

   addVariable(new Variable("Divider6LowCycles", Variable::Configuration));
   variables_["Divider6LowCycles"]->setDescription("Divider 6 Low Cycles.");
   variables_["Divider6LowCycles"]->setRange(0,15);

   addVariable(new Variable("Divider6HighCycles", Variable::Configuration));
   variables_["Divider6HighCycles"]->setDescription("Divider 6 High Cycles.");
   variables_["Divider6HighCycles"]->setRange(0,15);

   addVariable(new Variable("Divider6Bypass", Variable::Configuration));
   variables_["Divider6Bypass"]->setDescription("Divider 6 Bypass.");
   variables_["Divider6Bypass"]->setTrueFalse();

   addVariable(new Variable("Divider6NoSync", Variable::Configuration));
   variables_["Divider6NoSync"]->setDescription("Divider 6 No Sync.");
   variables_["Divider6NoSync"]->setTrueFalse();

   addVariable(new Variable("Divider6Force", Variable::Configuration));
   variables_["Divider6Force"]->setDescription("Divider 6 Force.");
   variables_["Divider6Force"]->setTrueFalse();

   addVariable(new Variable("Divider6StartHigh", Variable::Configuration));
   variables_["Divider6StartHigh"]->setDescription("Divider 6 Start High.");
   variables_["Divider6StartHigh"]->setTrueFalse();

   addVariable(new Variable("Divider6Phase", Variable::Configuration));
   variables_["Divider6Phase"]->setDescription("Divider 6 Phase Offset.");
   variables_["Divider6Phase"]->setRange(0,15);
}

// Deconstructor
Ad9510::~Ad9510 ( ) { }

// Method to write configuration registers
void Ad9510::writeConfig ( bool force ) {
   REGISTER_LOCK

   registers_["ChipPortConfig"]->set(0x90);

   registers_["Divider5RegA"]->set(variables_["Divider5LowCycles"]->getInt(),4,0xF);
   registers_["Divider5RegA"]->set(variables_["Divider5HighCycles"]->getInt(),0,0xF);
   registers_["Divider5RegB"]->set(variables_["Divider5Bypass"]->getInt(),7,0x1);
   registers_["Divider5RegB"]->set(variables_["Divider5NoSync"]->getInt(),6,0x1);
   registers_["Divider5RegB"]->set(variables_["Divider5Force"]->getInt(),5,0x1);
   registers_["Divider5RegB"]->set(variables_["Divider5StartHigh"]->getInt(),4,0x1);
   registers_["Divider5RegB"]->set(variables_["Divider5Phase"]->getInt(),0,0xF);

   registers_["Divider6RegA"]->set(variables_["Divider6LowCycles"]->getInt(),4,0xF);
   registers_["Divider6RegA"]->set(variables_["Divider6HighCycles"]->getInt(),0,0xF);
   registers_["Divider6RegB"]->set(variables_["Divider6Bypass"]->getInt(),7,0x1);
   registers_["Divider6RegB"]->set(variables_["Divider6NoSync"]->getInt(),6,0x1);
   registers_["Divider6RegB"]->set(variables_["Divider6Force"]->getInt(),5,0x1);
   registers_["Divider6RegB"]->set(variables_["Divider6StartHigh"]->getInt(),4,0x1);
   registers_["Divider6RegB"]->set(variables_["Divider6Phase"]->getInt(),0,0xF);

   if ( registers_["Divider5RegA"]->stale() || registers_["Divider5RegB"]->stale() || 
        registers_["Divider6RegA"]->stale() || registers_["Divider6RegB"]->stale() || force ) {

      writeRegister(registers_["Divider5RegA"],true);
      writeRegister(registers_["Divider5RegB"],true);
      writeRegister(registers_["Divider6RegA"],true);
      writeRegister(registers_["Divider6RegB"],true);
      registers_["UpdateRegister"]->set(0x1);
      writeRegister(registers_["UpdateRegister"],true);
   }

   REGISTER_UNLOCK
}

