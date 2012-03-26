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
   addRegister(new Register("TisClkReset",     0x01000001));
   addRegister(new Register("SyncRegister",    0x01000002));
   addRegister(new Register("HwAckEnable",     0x01000003));
   addRegister(new Register("TrigCntRst",      0x01000004));
   addRegister(new Register("TrigCount",       0x01000005));
   addRegister(new Register("SyncInEn",        0x01000006));
   addRegister(new Register("TrigDeadTime",    0x01000007));
   addRegister(new Register("FeReady",         0x01000008));
   addRegister(new Register("TisReady",        0x01000009));
   addRegister(new Register("TrigEnable",      0x0100000A));
   addRegister(new Register("SyncDelay",       0x0100000B));
   addRegister(new Register("IntTrigCount",    0x0100000C));
   addRegister(new Register("IntTrigPeriod",   0x0100000D));
   addRegister(new Register("DataBypass",      0x0100000E));
   addRegister(new Register("EventSize",       0x0100000F));
   addRegister(new Register("TrigDropCount",   0x01000010));
   addRegister(new Register("MasterReset",     0x01000100));

   // Setup variables
   addVariable(new Variable("FpgaVersion", Variable::Status));
   variables_["FpgaVersion"]->setDescription("FPGA version field");

   addVariable(new Variable("SyncRegister", Variable::Status));
   variables_["SyncRegister"]->setDescription("Sync Register");

   addVariable(new Variable("HwAckEnable", Variable::Configuration));
   variables_["HwAckEnable"]->setDescription("HW Ack Input Enable");
   variables_["HwAckEnable"]->setTrueFalse();

   addVariable(new Variable("TrigCount", Variable::Status));
   variables_["TrigCount"]->setDescription("Trig Counter");
   variables_["TrigCount"]->setComp(0,1,0,"");

   addVariable(new Variable("SyncInEn", Variable::Configuration));
   variables_["SyncInEn"]->setDescription("Sync Input Enable");
   variables_["SyncInEn"]->setTrueFalse();

   addVariable(new Variable("TrigDeadTime", Variable::Configuration));
   variables_["TrigDeadTime"]->setDescription("Trigger Dead Time");
   variables_["TrigDeadTime"]->setRange(0,0xFFFF);
   variables_["TrigDeadTime"]->setComp(0,4,0,"nS");

   addVariable(new Variable("FeReady", Variable::Configuration));
   variables_["FeReady"]->setDescription("FE Ready For Triggers");
   variables_["FeReady"]->setTrueFalse();

   addVariable(new Variable("TisReady", Variable::Status));
   variables_["TisReady"]->setDescription("TIS Logic Is Ready");
   variables_["TisReady"]->setTrueFalse();

   addVariable(new Variable("TrigEnable", Variable::Configuration));
   variables_["TrigEnable"]->setDescription("Use internal ti clock");
   variables_["TrigEnable"]->setTrueFalse();

   addVariable(new Variable("SyncDelay", Variable::Configuration));
   variables_["SyncDelay"]->setDescription("SYNC Input Delay");
   variables_["SyncDelay"]->setRange(0,63);
   variables_["SyncDelay"]->setComp(0,78,0,"pS");

   addVariable(new Variable("SyncInvert", Variable::Configuration));
   variables_["SyncInvert"]->setDescription("SYNC Input Invert");
   variables_["SyncInvert"]->setTrueFalse();

   addVariable(new Variable("SyncLatency", Variable::Configuration));
   variables_["SyncLatency"]->setDescription("SYNC Input Latency");
   variables_["SyncLatency"]->setRange(0,15);
   variables_["SyncLatency"]->setComp(0,4,0,"nS");

   addVariable(new Variable("TiClkDelay", Variable::Configuration));
   variables_["TiClkDelay"]->setDescription("TI Clock Input Delay");
   variables_["TiClkDelay"]->setRange(0,63);
   variables_["TiClkDelay"]->setComp(0,78,0,"pS");

   addVariable(new Variable("IntTrigCount", Variable::Configuration));
   variables_["IntTrigCount"]->setDescription("Internal trigger count");
   variables_["IntTrigCount"]->setComp(0,1,0,"");
   variables_["IntTrigCount"]->setRange(0,0x7FFFFFFF);

   addVariable(new Variable("IntTrigPeriod", Variable::Configuration));
   variables_["IntTrigPeriod"]->setDescription("Internal trigger period");
   variables_["IntTrigPeriod"]->setComp(0,8e-9,0,"S");
   variables_["IntTrigPeriod"]->setRange(0,0x7FFFFFFF);

   addVariable(new Variable("DataBypass", Variable::Configuration));
   variables_["DataBypass"]->setDescription("Data bypass");
   variables_["DataBypass"]->setTrueFalse();

   addVariable(new Variable("EventSize", Variable::Configuration));
   variables_["EventSize"]->setDescription("Event Sizes");
   variables_["EventSize"]->setRange(0,15);
   variables_["EventSize"]->setComp(0,1,1,"");

   addVariable(new Variable("TrigDropCount", Variable::Status));
   variables_["TrigDropCount"]->setDescription("Trigger Drop Counter");

   // Commands
   addCommand(new Command("TisSWTrig",0x02));
   commands_["TisSWTrig"]->setDescription("Generate TIS software trigger.");

   addCommand(new Command("IntTrigStart",0x04));
   commands_["IntTrigStart"]->setDescription("Start internal trigger sequence.");

   addCommand(new Command("TrigAck",0x01));
   commands_["TrigAck"]->setDescription("Ack block of triggers.");

   addCommand(new Command("TrigCntRst"));
   commands_["TrigCntRst"]->setDescription("Reset Trigger Counter.");

   addCommand(new Command("MasterReset"));
   commands_["MasterReset"]->setDescription("Master Reset.");

   addCommand(new Command("TisClkReset"));
   commands_["TisClkReset"]->setDescription("TIS Clock Reset.");

   addCommand(new Command("DropCountReset"));
   commands_["DropCountReset"]->setDescription("Drop count reset");

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
   else if ( name == "MasterReset" ) {
      REGISTER_LOCK
      writeRegister(registers_["MasterReset"],true,false);
      REGISTER_UNLOCK
   }
   else if ( name == "TisClkReset" ) {
      REGISTER_LOCK
      writeRegister(registers_["TisClkReset"],true);
      REGISTER_UNLOCK
   }
   else if ( name == "DropCountReset" ) {
      REGISTER_LOCK
      registers_["TrigDropCount"]->set(1);
      writeRegister(registers_["TrigDropCount"],true);
      registers_["TrigDropCount"]->set(0);
      writeRegister(registers_["TrigDropCount"],true);
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

   readRegister(registers_["TrigCount"]);
   variables_["TrigCount"]->setInt(registers_["TrigCount"]->get());

   readRegister(registers_["TisReady"]);
   variables_["TisReady"]->setInt(registers_["TisReady"]->get());

   readRegister(registers_["SyncRegister"]);
   variables_["SyncRegister"]->setInt(registers_["SyncRegister"]->get());

   readRegister(registers_["TrigDropCount"]);
   variables_["TrigDropCount"]->setInt(registers_["TrigDropCount"]->get());

   // Sub devices
   Device::readStatus();
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void TisFpga::readConfig () {
   REGISTER_LOCK

   // Read config
   readRegister(registers_["HwAckEnable"]);
   variables_["HwAckEnable"]->setInt(registers_["HwAckEnable"]->get());

   readRegister(registers_["TrigDeadTime"]);
   variables_["TrigDeadTime"]->setInt(registers_["TrigDeadTime"]->get());

   readRegister(registers_["SyncInEn"]);
   variables_["SyncInEn"]->setInt(registers_["SyncInEn"]->get());

   readRegister(registers_["FeReady"]);
   variables_["FeReady"]->setInt(registers_["FeReady"]->get());

   readRegister(registers_["TrigEnable"]);
   variables_["TrigEnable"]->setInt(registers_["TrigEnable"]->get());

   readRegister(registers_["SyncDelay"]);
   variables_["SyncDelay"]->setInt(registers_["SyncDelay"]->get(0,0x3f));
   variables_["SyncInvert"]->setInt(registers_["SyncDelay"]->get(7,0x1));
   variables_["SyncLatency"]->setInt(registers_["SyncDelay"]->get(8,0xf));
   variables_["TiClkDelay"]->setInt(registers_["SyncDelay"]->get(16,0x3f));

   readRegister(registers_["IntTrigCount"]);
   variables_["IntTrigCount"]->setInt(registers_["IntTrigCount"]->get());

   readRegister(registers_["IntTrigPeriod"]);
   variables_["IntTrigPeriod"]->setInt(registers_["IntTrigPeriod"]->get());

   readRegister(registers_["DataBypass"]);
   variables_["DataBypass"]->setInt(registers_["DataBypass"]->get());

   readRegister(registers_["EventSize"]);
   variables_["EventSize"]->setInt(registers_["EventSize"]->get());

   // Sub devices
   Device::readConfig();
   REGISTER_UNLOCK
}

// Method to write configuration registers
void TisFpga::writeConfig ( bool force ) {
   REGISTER_LOCK

   // Write config
   registers_["HwAckEnable"]->set(variables_["HwAckEnable"]->getInt());
   writeRegister(registers_["HwAckEnable"],force);

   registers_["SyncInEn"]->set(variables_["SyncInEn"]->getInt());
   writeRegister(registers_["SyncInEn"],force);

   registers_["TrigDeadTime"]->set(variables_["TrigDeadTime"]->getInt());
   writeRegister(registers_["TrigDeadTime"],force);

   registers_["FeReady"]->set(variables_["FeReady"]->getInt());
   writeRegister(registers_["FeReady"],force);

   registers_["TrigEnable"]->set(variables_["TrigEnable"]->getInt());
   writeRegister(registers_["TrigEnable"],force);

   registers_["SyncDelay"]->set(variables_["SyncDelay"]->getInt(),0,0x3f);
   registers_["SyncDelay"]->set(variables_["SyncInvert"]->getInt(),7,0x1);
   registers_["SyncDelay"]->set(variables_["SyncLatency"]->getInt(),8,0xf);
   registers_["SyncDelay"]->set(variables_["TiClkDelay"]->getInt(),16,0xff);
   writeRegister(registers_["SyncDelay"],force);

   registers_["IntTrigCount"]->set(variables_["IntTrigCount"]->getInt());
   writeRegister(registers_["IntTrigCount"],force);

   registers_["IntTrigPeriod"]->set(variables_["IntTrigPeriod"]->getInt());
   writeRegister(registers_["IntTrigPeriod"],force);

   registers_["DataBypass"]->set(variables_["DataBypass"]->getInt());
   writeRegister(registers_["DataBypass"],force);

   registers_["EventSize"]->set(variables_["EventSize"]->getInt());
   writeRegister(registers_["EventSize"],force);

   // Sub devices
   Device::writeConfig(force);
   REGISTER_UNLOCK
}

// Verify hardware state of configuration
void TisFpga::verifyConfig () {
   REGISTER_LOCK
   verifyRegister(registers_["HwAckEnable"]);
   verifyRegister(registers_["SyncInEn"]);
   verifyRegister(registers_["TrigDeadTime"]);
   verifyRegister(registers_["FeReady"]);
   verifyRegister(registers_["TrigEnable"]);
   verifyRegister(registers_["SyncDelay"]);
   verifyRegister(registers_["IntTrigCount"]);
   verifyRegister(registers_["IntTrigPeriod"]);
   verifyRegister(registers_["DataBypass"]);
   verifyRegister(registers_["EventSize"]);
   Device::verifyConfig();
   REGISTER_UNLOCK
}

