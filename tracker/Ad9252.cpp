//-----------------------------------------------------------------------------
// File          : Ad9252.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// AD9252 ADC
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <Ad9252.h>
#include <Register.h>
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
Ad9252::Ad9252 ( uint destination, uint baseAddress, uint index, Device *parent ) : 
                        Device(destination,baseAddress,"ad9252",index,parent) {

   // Description
   desc_ = "AD9252 ADC object.";

   // Create Registers: name, address
   addRegister(new Register("ChipPortConfig",  baseAddress_ + 0x00));
   addRegister(new Register("ChipId",          baseAddress_ + 0x01));
   addRegister(new Register("ChipGrade",       baseAddress_ + 0x02));
   addRegister(new Register("DeviceIndex2",    baseAddress_ + 0x04));
   addRegister(new Register("DeviceIndex1",    baseAddress_ + 0x05));
   addRegister(new Register("Modes",           baseAddress_ + 0x08));
   addRegister(new Register("Clock",           baseAddress_ + 0x09));
   addRegister(new Register("TestIo",          baseAddress_ + 0x0D));
   addRegister(new Register("OutputMode",      baseAddress_ + 0x14));
   addRegister(new Register("OutputAdjust",    baseAddress_ + 0x15));
   addRegister(new Register("OutputPhase",     baseAddress_ + 0x16));
   addRegister(new Register("UserPatt1Lsb",    baseAddress_ + 0x19));
   addRegister(new Register("UserPatt1Msb",    baseAddress_ + 0x1A));
   addRegister(new Register("UserPatt2Lsb",    baseAddress_ + 0x1B));
   addRegister(new Register("UserPatt2Msb",    baseAddress_ + 0x1C));
   addRegister(new Register("SerialControl",   baseAddress_ + 0x21));
   addRegister(new Register("SerialChStat",    baseAddress_ + 0x22));
   addRegister(new Register("DeviceUpdate",    baseAddress_ + 0xFF));

   // Variables
   addVariable(new Variable("ConfigEn", Variable::Configuration));
   variables_["ConfigEn"]->setDescription("Set to 'True' to enable register writes to ADC.");
   variables_["ConfigEn"]->setTrueFalse();
   variables_["ConfigEn"]->set("False");

   addVariable(new Variable("ChipId", Variable::Status));
   variables_["ChipId"]->setDescription("Read only chip ID value.");

   addVariable(new Variable("ChipGrade", Variable::Status));
   variables_["ChipGrade"]->setDescription("Read only chip grade value.");

   addVariable(new Variable("PowerDownMode", Variable::Configuration));
   variables_["PowerDownMode"]->setDescription("Set power mode of device.");
   vector<string> powerDownModes;
   powerDownModes.resize(4);
   powerDownModes[0]   = "ChipRun";
   powerDownModes[1]   = "FullPowerDown";
   powerDownModes[2]   = "Standby";
   powerDownModes[3]   = "Reset";
   variables_["PowerDownMode"]->setEnums(powerDownModes);

   addVariable(new Variable("DutyCycleStabilizer", Variable::Configuration));
   variables_["DutyCycleStabilizer"]->setDescription("Turns on internal duty cycle stabilizer. (default=True).");
   variables_["DutyCycleStabilizer"]->setTrueFalse();

   addVariable(new Variable("UserTestMode", Variable::Configuration));
   variables_["UserTestMode"]->setDescription("Sets user test mode of all channels.");
   vector<string> userTestModes;
   userTestModes.resize(4);
   userTestModes[0]    = "Off";
   userTestModes[1]    = "OnSingleAlt";
   userTestModes[2]    = "OnSingleOnce";
   userTestModes[3]    = "OnAlternateOnce";
   variables_["UserTestMode"]->setEnums(userTestModes);

   addVariable(new Variable("ResetPnLongGen", Variable::Configuration));
   variables_["ResetPnLongGen"]->setDescription("Reset PN long gen test mode.");
   variables_["ResetPnLongGen"]->setTrueFalse();

   addVariable(new Variable("ResetPnShortGen", Variable::Configuration));
   variables_["ResetPnShortGen"]->setDescription("Reset PN short gen test mode.");
   variables_["ResetPnShortGen"]->setTrueFalse();

   addVariable(new Variable("OutputTestMode", Variable::Configuration));
   variables_["OutputTestMode"]->setDescription("Set output test mode.");
   vector<string> outputTestModes;
   outputTestModes.resize(13);
   outputTestModes[0]  = "Off";
   outputTestModes[1]  = "MidscaleShort";
   outputTestModes[2]  = "PosFsShort";
   outputTestModes[3]  = "NegFsShort";
   outputTestModes[4]  = "Checkerboard";
   outputTestModes[5]  = "Pn23Seq";
   outputTestModes[6]  = "Pn9Seq";
   outputTestModes[7]  = "OneZeroWord";
   outputTestModes[8]  = "UserInput";
   outputTestModes[9]  = "OneZeroBit";
   outputTestModes[10] = "Sync1x";
   outputTestModes[11] = "OneBitHigh";
   outputTestModes[12] = "MixedBitFreq";
   variables_["OutputTestMode"]->setEnums(outputTestModes);

   addVariable(new Variable("OutputMode", Variable::Configuration));
   variables_["OutputMode"]->setDescription("Set output mode of device. Default=LVDS.");
   vector<string> outputModes;
   outputModes.resize(2);
   outputModes[0]      = "LvdsAnsi-644";
   outputModes[1]      = "LvdsLowPower";
   variables_["OutputMode"]->setEnums(outputModes);

   addVariable(new Variable("OutputInvert", Variable::Configuration));
   variables_["OutputInvert"]->setDescription("Enable output inversion.");
   variables_["OutputInvert"]->setTrueFalse();

   addVariable(new Variable("OutputFormat", Variable::Configuration));
   variables_["OutputFormat"]->setDescription("Set output format. binary or two's complement.");
   vector<string> outputFormats;
   outputFormats.resize(2);
   outputFormats[0]    = "OffsetBinary";
   outputFormats[1]    = "TwosCompliment";
   variables_["OutputFormat"]->setEnums(outputFormats);

   addVariable(new Variable("OutputTermDrive", Variable::Configuration));
   variables_["OutputTermDrive"]->setDescription("Set output driver termination.");
   vector<string> outputTerms;
   outputTerms.resize(4);
   outputTerms[0]      = "None";
   outputTerms[1]      = "Ohm200";
   outputTerms[2]      = "Ohm100";
   outputTerms[3]      = "Ohm100";
   variables_["OutputTermDrive"]->setEnums(outputTerms);

   addVariable(new Variable("DcoFcoDrive2x", Variable::Configuration));
   variables_["DcoFcoDrive2x"]->setDescription("Set DCO and DCO output drive strength.");
   variables_["DcoFcoDrive2x"]->setTrueFalse();

   addVariable(new Variable("OutputPhase", Variable::Configuration));
   variables_["OutputPhase"]->setDescription("Set output phase adjustment.");
   vector<string> outputPhases;
   outputPhases.resize(12);
   outputPhases[0]     = "Deg0";
   outputPhases[1]     = "Deg60";
   outputPhases[2]     = "Deg120";
   outputPhases[3]     = "Deg180";
   outputPhases[4]     = "Unused1";
   outputPhases[5]     = "Deg300";
   outputPhases[6]     = "Deg360";
   outputPhases[7]     = "Unused2";
   outputPhases[8]     = "Deg480";
   outputPhases[9]     = "Deg540";
   outputPhases[10]    = "Deg600";
   outputPhases[11]    = "Deg660";
   variables_["OutputPhase"]->setEnums(outputPhases);

   addVariable(new Variable("UserPattern1", Variable::Configuration));
   variables_["UserPattern1"]->setDescription("Set user test pattern 1 data.");

   addVariable(new Variable("UserPattern2", Variable::Configuration));
   variables_["UserPattern2"]->setDescription("Set user test pattern 2 data.");

   addVariable(new Variable("SerialLsbFirst", Variable::Configuration));
   variables_["SerialLsbFirst"]->setDescription("Set LSB first mode of device.");
   variables_["SerialLsbFirst"]->setTrueFalse();

   addVariable(new Variable("LowEncodeRate", Variable::Configuration));
   variables_["LowEncodeRate"]->setDescription("Set low rate less than 10mbs mode.");
   variables_["LowEncodeRate"]->setTrueFalse();

   addVariable(new Variable("SerialBits", Variable::Configuration));
   variables_["SerialBits"]->setDescription("Set number of serial bits.");
   vector<string> serialBits;
   serialBits.resize(5);
   serialBits[0]       = "Bits-14";
   serialBits[1]       = "Bits-8";
   serialBits[2]       = "Bits-10";
   serialBits[3]       = "Bits-12";
   serialBits[4]       = "Bits-14";
   variables_["SerialBits"]->setEnums(serialBits);

   addVariable(new Variable("ChanPowerDown", Variable::Configuration));
   variables_["ChanPowerDown"]->setDescription("Set channel power down.");
   variables_["ChanPowerDown"]->setTrueFalse();

}

// Deconstructor
Ad9252::~Ad9252 ( ) { }

// Method to read status registers and update variables
void Ad9252::readStatus ( ) {
   REGISTER_LOCK

   // Read registers
   readRegister(registers_["ChipId"]);
   variables_["ChipId"]->setInt(registers_["ChipId"]->get());

   readRegister(registers_["ChipGrade"]);
   variables_["ChipGrade"]->setInt(registers_["ChipGrade"]->get(4,0x7));

   REGISTER_UNLOCK
}

// Method to write configuration registers
void Ad9252::writeConfig ( bool force ) {

   // Writing is enabled?
   if ( variables_["ConfigEn"]->get() == "False" ) return;

   REGISTER_LOCK

   // Set registers
   registers_["ChipPortConfig"]->set(1,3,0x1);
   registers_["ChipPortConfig"]->set(1,4,0x1);
   writeRegister(registers_["ChipPortConfig"],force);

   registers_["DeviceIndex1"]->set(0x3F);
   writeRegister(registers_["DeviceIndex1"],force);

   registers_["DeviceIndex2"]->set(0x0F);
   writeRegister(registers_["DeviceIndex2"],force);

   registers_["Modes"]->set(variables_["PowerDownMode"]->getInt(),0,0x7);
   writeRegister(registers_["Modes"],force);

   registers_["Clock"]->set(variables_["DutyCycleStabilizer"]->getInt(),0,0x1);
   writeRegister(registers_["Clock"],force);

   registers_["TestIo"]->set(variables_["UserTestMode"]->getInt(),6,0x3);
   registers_["TestIo"]->set(variables_["ResetPnLongGen"]->getInt(),5,0x1);
   registers_["TestIo"]->set(variables_["ResetPnShortGen"]->getInt(),4,0x1);
   registers_["TestIo"]->set(variables_["OutputTestMode"]->getInt(),0,0xf);
   writeRegister(registers_["TestIo"],force);

   registers_["OutputMode"]->set(variables_["OutputMode"]->getInt(),6,0x1);
   registers_["OutputMode"]->set(variables_["OutputInvert"]->getInt(),2,0x1);
   registers_["OutputMode"]->set(variables_["OutputFormat"]->getInt(),0,0x3);
   writeRegister(registers_["OutputMode"],force);

   registers_["OutputAdjust"]->set(variables_["OutputTermDrive"]->getInt(),4,0x3);
   registers_["OutputAdjust"]->set(variables_["DcoFcoDrive2x"]->getInt(),0,0x1);
   writeRegister(registers_["OutputAdjust"],force);

   registers_["OutputPhase"]->set(variables_["OutputPhase"]->getInt(),0,0xF);
   writeRegister(registers_["OutputPhase"],force);

   registers_["UserPatt1Lsb"]->set(variables_["UserPattern1"]->getInt()&0xFF);
   writeRegister(registers_["UserPatt1Lsb"],force);

   registers_["UserPatt1Msb"]->set((variables_["UserPattern1"]->getInt()>>8)&0xFF);
   writeRegister(registers_["UserPatt1Msb"],force);

   registers_["UserPatt2Lsb"]->set(variables_["UserPattern2"]->getInt()&0xFF);
   writeRegister(registers_["UserPatt2Lsb"],force);

   registers_["UserPatt2Msb"]->set((variables_["UserPattern2"]->getInt()>>8)&0xFF);
   writeRegister(registers_["UserPatt2Msb"],force);

   registers_["SerialControl"]->set(variables_["SerialLsbFirst"]->getInt(),7,0x1);
   registers_["SerialControl"]->set(variables_["LowEncodeRate"]->getInt(),3,0x1);
   registers_["SerialControl"]->set(variables_["SerialBits"]->getInt(),0,0x7);
   writeRegister(registers_["SerialControl"],force);

   registers_["SerialChStat"]->set(variables_["ChanPowerDown"]->getInt(),0,0x1);
   writeRegister(registers_["SerialChStat"],force);

   registers_["DeviceUpdate"]->set(0x1);
   writeRegister(registers_["DeviceUpdate"],true);
   REGISTER_UNLOCK
}

