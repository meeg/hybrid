//-----------------------------------------------------------------------------
// File          : TisFpga.cpp
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
#include <TisFpga.h>
#include <Hybrid.h>
#include <Ad9510.h>
#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
TisFpga::TisFpga ( uint destination, uint index, Device *parent ) : 
                     Device(destination,0,"tisFpga",index,parent) {

   // Description
   desc_     = "TIS FPGA Object.";

   // Create Registers: name, address
   addRegister(new Register("Version",         0x01000000));
   addRegister(new Register("ScratchPad",      0x01000001));
   addRegister(new Register("GtpStatus",       0x01000002));
   addRegister(new Register("ExtEnable",       0x01000003));
   addRegister(new Register("TrigCntRst",      0x01000004));
   addRegister(new Register("TrigCount",       0x01000005));
   addRegister(new Register("TisEnable",       0x01000006));
   addRegister(new Register("TisLoopback",     0x01000007));
   addRegister(new Register("FeReady",         0x01000008));
   addRegister(new Register("TisReady",        0x01000009));
   addRegister(new Register("SyncDelay",       0x0100000A));

   // Setup variables
   addVariable(new Variable("FpgaVersion", Variable::Status));
   variables_["FpgaVersion"]->setDescription("FPGA version field");

   addVariable(new Variable("ScratchPad", Variable::Configuration));
   variables_["ScratchPad"]->setDescription("Scratchpad for testing");

   addVariable(new Variable("PllRxReady", Variable::Status));
   variables_["PllRxReady"]->setDescription("Pll Rx Is Ready");
   variables_["PllRxReady"]->setTrueFalse();

   addVariable(new Variable("PllTxReady", Variable::Status));
   variables_["PllTxReady"]->setDescription("Pll Tx Is Ready");
   variables_["PllTxReady"]->setTrueFalse();

   addVariable(new Variable("ExtEnable", Variable::Configuration));
   variables_["ExtEnable"]->setDescription("Ext Trig Enable");
   variables_["ExtEnable"]->setTrueFalse();

   addVariable(new Variable("TrigCount", Variable::Status));
   variables_["TrigCount"]->setDescription("Trig Counter");
   variables_["TrigCount"]->setComp(0,1,0,"");

   addVariable(new Variable("TisEnable", Variable::Configuration));
   variables_["TisEnable"]->setDescription("TIS Interface Enable");
   variables_["TisEnable"]->setTrueFalse();

   addVariable(new Variable("TisLoopback", Variable::Configuration));
   variables_["TisLoopback"]->setDescription("TIS Interface Loopback");
   variables_["TisLoopback"]->setTrueFalse();

   addVariable(new Variable("FeReady", Variable::Configuration));
   variables_["FeReady"]->setDescription("FE Ready For Triggers");
   variables_["FeReady"]->setTrueFalse();

   addVariable(new Variable("TisReady", Variable::Status));
   variables_["TisReady"]->setDescription("TIS Logic Is Ready");
   variables_["TisReady"]->setTrueFalse();

   addVariable(new Variable("SyncDelay", Variable::Configuration));
   variables_["SyncDelay"]->setDescription("SYNC Input Delay");
   variables_["SyncDelay"]->setRange(0,63);
   variables_["SyncDelay"]->setComp(0,78,0,"pS");

   // Commands
   addCommand(new Command("TisSWTrig",0x0));
   commands_["TisSWTrig"]->setDescription("Generate TIS software trigger.");

   addCommand(new Command("TrigCntRst"));
   commands_["TrigCntRst"]->setDescription("Reset Trigger Counter.");

   // Devices
   addDevice(new Ad9510(destination,0x01002000,0,this));
}

// Deconstructor
TisFpga::~TisFpga ( ) { }

// Method to process a command
void TisFpga::command ( string name, string arg) {

   if ( name == "TrigCntRst" ) {
      REGISTER_LOCK
      registers_["TrigCntRst"]->set(0x1);
      writeRegister(registers_["TrigCntRst"],true);
      REGISTER_UNLOCK
   }
   else Device::command(name, arg);
}

// Method to read status registers and update variables
void TisFpga::readStatus ( ) {
   REGISTER_LOCK

   // Read status
   readRegister(registers_["Version"]);
   variables_["FpgaVersion"]->setInt(registers_["Version"]->get());

   readRegister(registers_["GtpStatus"]);
   variables_["PllRxReady"]->setInt(registers_["GtpStatus"]->get(0,0x1));
   variables_["PllTxReady"]->setInt(registers_["GtpStatus"]->get(1,0x1));

   readRegister(registers_["TrigCount"]);
   variables_["TrigCount"]->setInt(registers_["TrigCount"]->get());

   readRegister(registers_["TisReady"]);
   variables_["TisReady"]->setInt(registers_["TisReady"]->get());

   // Sub devices
   Device::readStatus();
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void TisFpga::readConfig () {
   REGISTER_LOCK

   // Read config
   readRegister(registers_["ScratchPad"]);
   variables_["ScratchPad"]->setInt(registers_["ScratchPad"]->get());

   readRegister(registers_["ExtEnable"]);
   variables_["ExtEnable"]->setInt(registers_["ExtEnable"]->get());

   readRegister(registers_["TisEnable"]);
   variables_["TisEnable"]->setInt(registers_["TisEnable"]->get());

   readRegister(registers_["TisLoopback"]);
   variables_["TisLoopback"]->setInt(registers_["TisLoopback"]->get());

   readRegister(registers_["FeReady"]);
   variables_["FeReady"]->setInt(registers_["FeReady"]->get());

   readRegister(registers_["SyncDelay"]);
   variables_["SyncDelay"]->setInt(registers_["SyncDelay"]->get());

   // Sub devices
   Device::readConfig();
   REGISTER_UNLOCK
}

// Method to write configuration registers
void TisFpga::writeConfig ( bool force ) {
   REGISTER_LOCK

   // Write config
   registers_["ScratchPad"]->set(variables_["ScratchPad"]->getInt());
   writeRegister(registers_["ScratchPad"],force);

   registers_["ExtEnable"]->set(variables_["ExtEnable"]->getInt());
   writeRegister(registers_["ExtEnable"],force);

   registers_["TisEnable"]->set(variables_["TisEnable"]->getInt());
   writeRegister(registers_["TisEnable"],force);

   registers_["TisLoopback"]->set(variables_["TisLoopback"]->getInt());
   writeRegister(registers_["TisLoopback"],force);

   registers_["FeReady"]->set(variables_["FeReady"]->getInt());
   writeRegister(registers_["FeReady"],force);

   registers_["SyncDelay"]->set(variables_["SyncDelay"]->getInt());
   writeRegister(registers_["SyncDelay"],force);

   // Sub devices
   Device::writeConfig(force);
   REGISTER_UNLOCK
}

// Verify hardware state of configuration
void TisFpga::verifyConfig () {
   REGISTER_LOCK
   verifyRegister(registers_["ScratchPad"]);
   verifyRegister(registers_["ExtEnable"]);
   verifyRegister(registers_["TisEnable"]);
   verifyRegister(registers_["TisLoopback"]);
   verifyRegister(registers_["FeReady"]);
   verifyRegister(registers_["SyncDelay"]);
   Device::verifyConfig();
   REGISTER_UNLOCK
}

