//-----------------------------------------------------------------------------
// File          : FirmwareVersion.cpp
// Author        : Ben Reese <bareese@slac.stanford.edu>
// Created       : 11/14/2013
// Project       : HPS SVT
//-----------------------------------------------------------------------------
// Description :
// Device container for AxiVersion.vhd
//-----------------------------------------------------------------------------
// Copyright (c) 2013 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 11/14/2013: created
//-----------------------------------------------------------------------------
#include <FirmwareVersion.h>
#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
FirmwareVersion::FirmwareVersion ( uint destination, uint baseAddress, uint index, Device *parent ) : 
                        Device(destination,baseAddress,"FirmwareVersion",index,parent) {

   // Description
   desc_ = "Firmware Version object.";

   // Create Registers: name, address
   addRegister(new Register("FpgaVersion",   baseAddress_ + 0x00));
   addRegister(new Register("ScratchPad",    baseAddress_ + 0x01));
   addRegister(new Register("DeviceDnaHigh", baseAddress_ + 0x02));
   addRegister(new Register("DeviceDnaLow",  baseAddress_ + 0x03));
   addRegister(new Register("FdSerialHigh",  baseAddress_ + 0x04));
   addRegister(new Register("FdSerialLow",   baseAddress_ + 0x05));
   addRegister(new Register("MasterReset",   baseAddress_ + 0x06));
   addRegister(new Register("FpgaReload",    baseAddress_ + 0x07));

   addRegister(new Register("UserConstants", baseAddress_ + 0x100, 64));
   addRegister(new Register("BuildStamp",    baseAddress_ + 0x200, 64));

   // Variables
   Variable* v;

   v = getVariable("Enabled");
   v->set("True");
   v->setHidden(true);

   v = new Variable("FpgaVersion", Variable::Status);
   v->setDescription("FPGA Firmware Version Number");
   addVariable(v);

   v = new Variable("ScratchPad", Variable::Configuration);
   v->setDescription("Register to test reads and writes");
   v->setPerInstance(true);
   addVariable(v);

   v = new Variable("DeviceDna", Variable::Status);
   v->setDescription("Xilinx Device DNA value burned into FPGA");
   addVariable(v);

   v = new Variable("FdSerial", Variable::Status);
   v->setDescription("Board ID value read from DS2411 chip");
   addVariable(v);

   v = new Variable("BuildStamp", Variable::Status);
   v->setDescription("Firmware Build String");
   addVariable(v);

   //Commands
   Command *c;

   c = new Command("MasterReset");
   c->setDescription("Perform a logical reset of every FPGA register");
   addCommand(c);

   c = new Command("FpgaReload");
   c->setDescription("Reload the FPGA from the attached PROM");
   addCommand(c);

}

// Deconstructor
FirmwareVersion::~FirmwareVersion ( ) { }

// Process Commands
void FirmwareVersion::command(string name, string arg) {
   Register *r;
   if (name == "MasterReset") {
      REGISTER_LOCK
      r = getRegister("MasterReset");
      r->set(0x1);
      writeRegister(r, true, false);
      REGISTER_UNLOCK
   }
   else if (name == "FpgaReload") {
      REGISTER_LOCK
      r = getRegister("FpgaReload");
      r->set(0x1);
      writeRegister(r, true, false);
      REGISTER_UNLOCK
   }
}

// Method to read status registers and update variables
void FirmwareVersion::readStatus ( ) {
   REGISTER_LOCK
         
   stringstream ss;
   
   // Read registers
   readRegister(getRegister("FpgaVersion"));
   getVariable("FpgaVersion")->setInt(getRegister("FpgaVersion")->get());

   readRegister(getRegister("DeviceDnaHigh"));
   readRegister(getRegister("DeviceDnaLow"));
   ss.str("");
   ss << "0x" << hex << setw(8) << setfill('0');
   ss << getRegister("DeviceDnaHigh")->get();
   ss << getRegister("DeviceDnaLow")->get();
   getVariable("DeviceDna")->set(ss.str());

   readRegister(getRegister("FdSerialHigh"));
   readRegister(getRegister("FdSerialLow"));
   ss.str("");
   ss << "0x" << hex << setw(8) << setfill('0');
   ss << getRegister("FdSerialHigh")->get();
   ss << getRegister("FdSerialLow")->get();
   getVariable("FdSerial")->set(ss.str());

   readRegister(getRegister("BuildStamp"));
   string tmp = string((char *)(getRegister("BuildStamp")->data()));
   getVariable("BuildStamp")->set(tmp);

   REGISTER_UNLOCK
}

void FirmwareVersion::readConfig ( ) {
   REGISTER_LOCK

   readRegister(getRegister("ScratchPad"));
   getVariable("ScratchPad")->setInt(getRegister("ScratchPad")->get());

   REGISTER_UNLOCK
}

// Method to write configuration registers
void FirmwareVersion::writeConfig ( bool force ) {
   REGISTER_LOCK

   // Set registers
   getRegister("ScratchPad")->set(getVariable("ScratchPad")->getInt());
   writeRegister(getRegister("ScratchPad"), force);

   REGISTER_UNLOCK
}

