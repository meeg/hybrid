//-----------------------------------------------------------------------------
// File          : Apv25.cpp
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
#include <Apv25.h>
#include <Register.h>
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
Apv25::Apv25 ( uint destination, uint baseAddress, uint index, Device *parent ) : 
                        Device(destination,baseAddress,"apv25",index,parent) {

   // Description
   desc_ = "APV25 Object.";

   // Create Registers: name, address
   addRegister(new Register("Error",   baseAddress_ + 0x00));
   addRegister(new Register("MuxGain", baseAddress_ + 0x03));
   addRegister(new Register("Latency", baseAddress_ + 0x02));
   addRegister(new Register("Mode",    baseAddress_ + 0x01));
   addRegister(new Register("Csel",    baseAddress_ + 0x1D));
   addRegister(new Register("Cdrv",    baseAddress_ + 0x1C));
   addRegister(new Register("Vpsp",    baseAddress_ + 0x1B));
   addRegister(new Register("Vfs",     baseAddress_ + 0x1A));
   addRegister(new Register("Vfp",     baseAddress_ + 0x19));
   addRegister(new Register("Ical",    baseAddress_ + 0x18));
   addRegister(new Register("Ispare",  baseAddress_ + 0x17));
   addRegister(new Register("ImuxIn",  baseAddress_ + 0x16));
   addRegister(new Register("Ipsp",    baseAddress_ + 0x15));
   addRegister(new Register("Issf",    baseAddress_ + 0x14));
   addRegister(new Register("Isha",    baseAddress_ + 0x13));
   addRegister(new Register("Ipsf",    baseAddress_ + 0x12));
   addRegister(new Register("Ipcasc",  baseAddress_ + 0x11));
   addRegister(new Register("Ipre",    baseAddress_ + 0x10));

   registers_["Mode"]->set(1,0,0x1);
   registers_["Mode"]->clrStale();

   // Create variables
   addVariable(new Variable("FifoError", Variable::Status));
   variables_["FifoError"]->setDescription("Fifo error status.");
   variables_["FifoError"]->setTrueFalse();

   addVariable(new Variable("LatencyError", Variable::Status));
   variables_["LatencyError"]->setDescription("Latency error status.");
   variables_["LatencyError"]->setTrueFalse();

   addVariable(new Variable("PreampPolarity", Variable::Configuration));
   variables_["PreampPolarity"]->setDescription("Set pre-amp input polarity.");
   vector<string> polarities;
   polarities.resize(2);
   polarities[0]  = "NonInverting";
   polarities[1]  = "Inverting";
   variables_["PreampPolarity"]->setEnums(polarities);

   addVariable(new Variable("ReadOutFrequency", Variable::Configuration));
   variables_["ReadOutFrequency"]->setDescription("Set readout frequency to enable muxing.");
   vector<string> frequencies;
   frequencies.resize(2);
   frequencies[0] = "Mhz20";
   frequencies[1] = "Mhz40";
   variables_["ReadOutFrequency"]->setEnums(frequencies);

   addVariable(new Variable("ReadOutMode", Variable::Configuration));
   variables_["ReadOutMode"]->setDescription("Set readout mode.");
   vector<string> readModes;
   readModes.resize(2);
   readModes[0]   = "Deconvolution";
   readModes[1]   = "Peak";
   variables_["ReadOutMode"]->setEnums(readModes);

   addVariable(new Variable("CalibInhibit", Variable::Configuration));
   variables_["CalibInhibit"]->setDescription("Set calibration inhibit.");
   variables_["CalibInhibit"]->setTrueFalse();

   addVariable(new Variable("TriggerMode", Variable::Configuration));
   variables_["TriggerMode"]->setDescription("Set number of samples in trigger.");
   vector<string> trigModes;
   trigModes.resize(2);
   trigModes[0]   = "Sample3";
   trigModes[1]   = "Sample1";
   variables_["TriggerMode"]->setEnums(trigModes);

   addVariable(new Variable("MuxGain", Variable::Configuration));
   variables_["MuxGain"]->setDescription("Determine output mux stage gain.");
   vector<string> muxGains;
   muxGains.resize(5);
   muxGains[0] = "Mip_0_8mA";
   muxGains[1] = "Mip_0_9mA";
   muxGains[2] = "Mip_1_0mA";
   muxGains[3] = "Mip_1_1mA";
   muxGains[4] = "Mip_1_2mA";
   variables_["MuxGain"]->setEnums(muxGains);
   registers_["MuxGain"]->set(0x01);
   registers_["MuxGain"]->clrStale();

   addVariable(new Variable("Latency", Variable::Configuration));
   variables_["Latency"]->setDescription("Determine the distance between read and write pointers.\n" 
                                         "8-bit value. 0 - 255");
   variables_["Latency"]->setComp(0,25,0,"nS");
   variables_["Latency"]->setRange(0,255);

   addVariable(new Variable("Csel", Variable::Configuration));
   variables_["Csel"]->setDescription("Number of delays (3.125ns) to insert from edge of calibration generator.");
   vector<string> cselValues;
   cselValues.resize(9);
   cselValues[0] = "Dly_0x3_125ns";
   cselValues[1] = "Dly_1x3_125ns";
   cselValues[2] = "Dly_2x3_125ns";
   cselValues[3] = "Dly_3x3_125ns";
   cselValues[4] = "Dly_4x3_125ns";
   cselValues[5] = "Dly_5x3_125ns";
   cselValues[6] = "Dly_6x3_125ns";
   cselValues[7] = "Dly_7x3_125ns";
   cselValues[8] = "Dly_8x3_125ns";
   variables_["Csel"]->setEnums(cselValues);

   addVariable(new Variable("CalGroup", Variable::Configuration));
   variables_["CalGroup"]->setDescription("Select which set of 16 channels to calibrate.\n"
                                          "Valid range is 0 to 7.");
   variables_["CalGroup"]->setRange(0,7);
   registers_["Cdrv"]->set(0xfe);
   registers_["Cdrv"]->clrStale();

   addVariable(new Variable("Vpsp", Variable::Configuration));
   variables_["Vpsp"]->setDescription("APSP voltage level adjust.\n"
                                      "8-bit value. 0 - 255");
   variables_["Vpsp"]->setRange(0,255);
   variables_["Vpsp"]->setComp(0,-0.0075,1.25,"V");

   addVariable(new Variable("Vfs", Variable::Configuration));
   variables_["Vfs"]->setDescription("Shaper feedback voltage bias.\n"
                                     "8-bit value. 0 - 255");
   variables_["Vfs"]->setRange(0,255);
   variables_["Vfs"]->setComp(0,0.0075,-1.25,"V");

   addVariable(new Variable("Vfp", Variable::Configuration));
   variables_["Vfp"]->setDescription("Preamp feedback voltage bias.\n"
                                     "8-bit value. 0 - 255");
   variables_["Vfp"]->setRange(0,255);
   variables_["Vfp"]->setComp(0,0.0075,-1.25,"V");

   addVariable(new Variable("Ical", Variable::Configuration));
   variables_["Ical"]->setDescription("Calibrate edge generator current bias\n"
                                      "8-bit value. 0 - 255");
   variables_["Ical"]->setRange(0,255);
   variables_["Ical"]->setComp(0,625,0,"el");

   addVariable(new Variable("Ispare", Variable::Configuration));
   variables_["Ispare"]->setDescription("Spare");
   variables_["Ispare"]->setHidden(true);

   addVariable(new Variable("ImuxIn", Variable::Configuration));
   variables_["ImuxIn"]->setDescription("Multiplexer input current bias.\n"
                                        "8-bit value. 0 - 255");
   variables_["ImuxIn"]->setRange(0,255);
   variables_["ImuxIn"]->setComp(0,1,0,"uA");

   addVariable(new Variable("Ipsp",             Variable::Configuration));
   variables_["Ipsp"]->setDescription("APSP current bias.\n"
                                      "8-bit value. 0 - 255");
   variables_["Ipsp"]->setRange(0,255);
   variables_["Ipsp"]->setComp(0,1,0,"uA");

   addVariable(new Variable("Issf", Variable::Configuration));
   variables_["Issf"]->setDescription("Shaper source follower current bias.\n"
                                      "8-bit value. 0 - 255");
   variables_["Issf"]->setRange(0,255);
   variables_["Issf"]->setComp(0,1,0,"uA");

   addVariable(new Variable("Isha", Variable::Configuration));
   variables_["Isha"]->setDescription("Shaper input FET current bias.\n"
                                      "8-bit value. 0 - 255");
   variables_["Isha"]->setRange(0,255);
   variables_["Isha"]->setComp(0,1,0,"uA");

   addVariable(new Variable("Ipsf", Variable::Configuration));
   variables_["Ipsf"]->setDescription("Shaper source follower current bias.\n"
                                      "8-bit value. 0 - 255");
   variables_["Ipsf"]->setRange(0,255);
   variables_["Ipsf"]->setComp(0,1,0,"uA");

   addVariable(new Variable("Ipcasc", Variable::Configuration));
   variables_["Ipcasc"]->setDescription("Preamp cascode current bias.\n"
                                        "8-bit value. 0 - 255");
   variables_["Ipcasc"]->setRange(0,255);
   variables_["Ipcasc"]->setComp(0,1,0,"uA");

   addVariable(new Variable("Ipre", Variable::Configuration));
   variables_["Ipre"]->setDescription("Peramp input FET current bias.\n"
                                      "8-bit value. 0 - 255");
   variables_["Ipre"]->setRange(0,255);
   variables_["Ipre"]->setComp(0,4,0,"uA");

}

// Deconstructor
Apv25::~Apv25 ( ) { }



// Method to read status registers and update variables
void Apv25::readStatus ( ) {

   REGISTER_LOCK

   // Read status
   readRegister(registers_["Error"]);
   variables_["FifoError"]->setInt(registers_["Error"]->get(1,0x1));
   variables_["LatencyError"]->setInt(registers_["Error"]->get(0,0x1));

   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void Apv25::readConfig (  ) {
   uint idx;

   REGISTER_LOCK

   // Read Configuration
   readRegister(registers_["MuxGain"]);
   if      ( registers_["MuxGain"]->get(0,0x1) == 1 ) variables_["MuxGain"]->setInt(0);
   else if ( registers_["MuxGain"]->get(1,0x1) == 1 ) variables_["MuxGain"]->setInt(1);
   else if ( registers_["MuxGain"]->get(2,0x1) == 1 ) variables_["MuxGain"]->setInt(2);
   else if ( registers_["MuxGain"]->get(3,0x1) == 1 ) variables_["MuxGain"]->setInt(3);
   else if ( registers_["MuxGain"]->get(4,0x1) == 1 ) variables_["MuxGain"]->setInt(4);
   else variables_["MuxGain"]->setInt(0);

   readRegister(registers_["Latency"]);
   variables_["Latency"]->setInt(registers_["Latency"]->get());

   readRegister(registers_["Mode"]);
   variables_["PreampPolarity"]->setInt(registers_["Mode"]->get(5,0x1));
   variables_["ReadOutFrequency"]->setInt(registers_["Mode"]->get(4,0x1));
   variables_["ReadOutMode"]->setInt(registers_["Mode"]->get(3,0x1));
   variables_["CalibInhibit"]->setInt(registers_["Mode"]->get(2,0x1));
   variables_["TriggerMode"]->setInt(registers_["Mode"]->get(1,0x1));

   readRegister(registers_["Csel"]);
   if      ( registers_["Csel"]->get(0,0x1) == 0 ) variables_["Csel"]->setInt(1);
   else if ( registers_["Csel"]->get(1,0x1) == 0 ) variables_["Csel"]->setInt(2);
   else if ( registers_["Csel"]->get(2,0x1) == 0 ) variables_["Csel"]->setInt(3);
   else if ( registers_["Csel"]->get(3,0x1) == 0 ) variables_["Csel"]->setInt(4);
   else if ( registers_["Csel"]->get(4,0x1) == 0 ) variables_["Csel"]->setInt(5);
   else if ( registers_["Csel"]->get(5,0x1) == 0 ) variables_["Csel"]->setInt(6);
   else if ( registers_["Csel"]->get(6,0x1) == 0 ) variables_["Csel"]->setInt(7);
   else if ( registers_["Csel"]->get(7,0x1) == 0 ) variables_["Csel"]->setInt(8);
   else variables_["Csel"]->setInt(0);

   readRegister(registers_["Cdrv"]);
   variables_["CalGroup"]->setInt(0);
   for (idx=0; idx < 8; idx++) {
     if ( registers_["Cdrv"]->get(idx,0x1) == 0 ) {
        variables_["CalGroup"]->setInt(idx);
        break;
     }
   }

   readRegister(registers_["Vpsp"]);
   variables_["Vpsp"]->setInt(registers_["Vpsp"]->get());

   readRegister(registers_["Vfs"]);
   variables_["Vfs"]->setInt(registers_["Vfs"]->get());

   readRegister(registers_["Vfp"]);
   variables_["Vfp"]->setInt(registers_["Vfp"]->get());

   readRegister(registers_["Ical"]);
   variables_["Ical"]->setInt(registers_["Ical"]->get());

   readRegister(registers_["Ispare"]);
   variables_["Ispare"]->setInt(registers_["Ispare"]->get());

   readRegister(registers_["ImuxIn"]);
   variables_["ImuxIn"]->setInt(registers_["ImuxIn"]->get());

   readRegister(registers_["Ipsp"]);
   variables_["Ipsp"]->setInt(registers_["Ipsp"]->get());

   readRegister(registers_["Issf"]);
   variables_["Issf"]->setInt(registers_["Issf"]->get());

   readRegister(registers_["Isha"]);
   variables_["Isha"]->setInt(registers_["Isha"]->get());

   readRegister(registers_["Ipsf"]);
   variables_["Ipsf"]->setInt(registers_["Ipsf"]->get());

   readRegister(registers_["Ipcasc"]);
   variables_["Ipcasc"]->setInt(registers_["Ipcasc"]->get());

   readRegister(registers_["Ipre"]);
   variables_["Ipre"]->setInt(registers_["Ipre"]->get());

   REGISTER_UNLOCK
}

// Method to write configuration registers
void Apv25::writeConfig ( bool force ) {
   uint temp;
   uint idx;

   REGISTER_LOCK

   // Set registers
   switch ( variables_["MuxGain"]->getInt() ) {
      case  0: temp = 0x01; break;
      case  1: temp = 0x02; break;
      case  2: temp = 0x04; break;
      case  3: temp = 0x08; break;
      case  4: temp = 0x10; break;
      default: temp = 0x02; break;
   }
   registers_["MuxGain"]->set(temp);

   registers_["Latency"]->set(variables_["Latency"]->getInt());
   registers_["Mode"]->set(variables_["PreampPolarity"]->getInt(),5,0x1);
   registers_["Mode"]->set(variables_["ReadOutFrequency"]->getInt(),4,0x1);
   registers_["Mode"]->set(variables_["ReadOutMode"]->getInt(),3,0x1);
   registers_["Mode"]->set(variables_["CalibInhibit"]->getInt(),2,0x1);
   registers_["Mode"]->set(variables_["TriggerMode"]->getInt(),1,0x1);

   switch ( variables_["Csel"]->getInt() ) {
      case  0: temp = 0xFF; break;
      case  1: temp = 0xFE; break;
      case  2: temp = 0xFD; break;
      case  3: temp = 0xFB; break;
      case  4: temp = 0xF7; break;
      case  5: temp = 0xEF; break;
      case  6: temp = 0xDF; break;
      case  7: temp = 0xBF; break;
      case  8: temp = 0x7F; break;
      default: temp = 0xFF; break;
   }
   registers_["Csel"]->set(temp);

   idx = (variables_["CalGroup"]->getInt() & 0x7);
   temp = 0xFF ^ (0x1 << idx);
   registers_["Cdrv"]->set(temp);

   registers_["Vpsp"]->set(variables_["Vpsp"]->getInt());
   registers_["Vfs"]->set(variables_["Vfs"]->getInt());
   registers_["Vfp"]->set(variables_["Vfp"]->getInt());
   registers_["Ical"]->set(variables_["Ical"]->getInt());
   registers_["Ispare"]->set(variables_["Ispare"]->getInt());
   registers_["ImuxIn"]->set(variables_["ImuxIn"]->getInt());
   registers_["Ipsp"]->set(variables_["Ipsp"]->getInt());
   registers_["Issf"]->set(variables_["Issf"]->getInt());
   registers_["Isha"]->set(variables_["Isha"]->getInt());
   registers_["Ipsf"]->set(variables_["Ipsf"]->getInt());
   registers_["Ipcasc"]->set(variables_["Ipcasc"]->getInt());
   registers_["Ipre"]->set(variables_["Ipre"]->getInt());

   // Disable bias if any of the bias values will be updated
   if ( force || registers_["Vpsp"]->stale()   || registers_["Vfs"]->stale() ||
                 registers_["Vfp"]->stale()    || registers_["Ical"]->stale() ||
                 registers_["Ispare"]->stale() || registers_["ImuxIn"]->stale() ||
                 registers_["Ipsp"]->stale()   || registers_["Issf"]->stale() ||
                 registers_["Isha"]->stale()   || registers_["Ipsf"]->stale() ||
                 registers_["Ipcasc"]->stale() || registers_["Ipre"]->stale() ) {

      // Disable Bias
      registers_["Mode"]->set(0,0,0x1);
      writeRegister(registers_["Mode"],true);
   }

   // Set registers
   writeRegister(registers_["MuxGain"],force);
   writeRegister(registers_["Latency"],force);
   writeRegister(registers_["Mode"],force);
   writeRegister(registers_["Csel"],force);
   writeRegister(registers_["Cdrv"],force);
   writeRegister(registers_["Vpsp"],force);
   writeRegister(registers_["Vfs"],force);
   writeRegister(registers_["Vfp"],force);
   writeRegister(registers_["Ical"],force);
   writeRegister(registers_["Ispare"],force);
   writeRegister(registers_["ImuxIn"],force);
   writeRegister(registers_["Ipsp"],force);
   writeRegister(registers_["Issf"],force);
   writeRegister(registers_["Isha"],force);
   writeRegister(registers_["Ipsf"],force);
   writeRegister(registers_["Ipcasc"],force);
   writeRegister(registers_["Ipre"],force);

   // Enable Bias set mode register
   registers_["Mode"]->set(1,0,0x1);
   writeRegister(registers_["Mode"],force);

   REGISTER_UNLOCK

}

// Verify hardware state of configuration
void Apv25::verifyConfig ( ) {

   REGISTER_LOCK

   // Read Configuration
   verifyRegister(registers_["MuxGain"]);
   verifyRegister(registers_["Latency"]);
   verifyRegister(registers_["Mode"]);
   verifyRegister(registers_["Csel"]);
   verifyRegister(registers_["Cdrv"]);
   verifyRegister(registers_["Vpsp"]);
   verifyRegister(registers_["Vfs"]);
   verifyRegister(registers_["Vfp"]);
   verifyRegister(registers_["Ical"]);
   verifyRegister(registers_["Ispare"]);
   verifyRegister(registers_["ImuxIn"]);
   verifyRegister(registers_["Ipsp"]);
   verifyRegister(registers_["Issf"]);
   verifyRegister(registers_["Isha"]);
   verifyRegister(registers_["Ipsf"]);
   verifyRegister(registers_["Ipcasc"]);
   verifyRegister(registers_["Ipre"]);

   REGISTER_UNLOCK
}

