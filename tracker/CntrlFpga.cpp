//-----------------------------------------------------------------------------
// File          : CntrlFpga.cpp
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
#include <CntrlFpga.h>
#include <Hybrid.h>
#include <Ad9252.h>
#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <math.h>
using namespace std;

// Constructor
CntrlFpga::CntrlFpga ( uint destination, uint index, Device *parent ) : 
                     Device(destination,0,"cntrlFpga",index,parent) {

   double       temp;
   double       tk;
   double       res;
   double       volt;
   unsigned int idx;
   unsigned int hyb, apv;
   stringstream regName;

   // Fill temperature lookup table
   temp = minTemp_;
   while ( temp < maxTemp_ ) {
      tk = k0_ + temp;
      //res = t25_ * exp(coeffA_+(coeffB_/tk)+(coeffC_/(tk*tk))+(coeffD_/(tk*tk*tk)));      
      res = constA_ * exp(beta_/tk);
      volt = (res*vmax_)/(rdiv_+res);
      idx = (uint)((volt / vref_) * (double)(adcCnt_-1));
      if ( idx < adcCnt_ ) tempTable_[idx] = temp; 
      //cout << "temp=" << temp << " adc=0x" << hex << idx << endl;
      temp += incTemp_;
   }

   // Description
   desc_ = "Control FPGA Object.";

   // Init threshold data
   thold_ = NULL;
   filt_  = NULL;

   variables_["Enabled"]->set("False");

   // Create Registers: name, address
   addRegister(new Register("Version",         0x01000000));
   addRegister(new Register("MasterReset",     0x01000001));
   addRegister(new Register("TrigCount",       0x01000002));
   addRegister(new Register("AdcChanEn",       0x01000003));
   addRegister(new Register("Apv25Reset",      0x01000004));
   addRegister(new Register("ApvClkRst",       0x01000005));
   addRegister(new Register("TisClkEn",        0x01000006));
   addRegister(new Register("TrigCntRst",      0x01000007));
   addRegister(new Register("ApvSyncStatus",   0x01000008));
   addRegister(new Register("ApvHardReset",    0x01000009));
   addRegister(new Register("TholdEnable",     0x0100000A));
   addRegister(new Register("ApvTrigGenPause", 0x0100000B));
   addRegister(new Register("ApvTrigSrcType",  0x0100000C));
   addRegister(new Register("ClockSelect",     0x0100000E));
   addRegister(new Register("TempData",        0x01000010, 6));
   addRegister(new Register("TempPollPer",     0x01000016));
   addRegister(new Register("CalDelay",        0x01000017));
   addRegister(new Register("InputDelay",      0x01000020, 15));
   addRegister(new Register("FrameDelayA",     0x01000030));
   addRegister(new Register("FrameDelayB",     0x01000031));
   addRegister(new Register("AdcClkDelay",     0x01000032));
   addRegister(new Register("SyncData",        0x01000040, 15));
   addRegister(new Register("TrigEdgeNeg",     0x01000050));
   addRegister(new Register("ThresholdA",      0x01200000, 640)); // Hybrid 0
   addRegister(new Register("ThresholdB",      0x01200400, 640)); // Hybrid 1
   addRegister(new Register("ThresholdC",      0x01200800, 640)); // Hybrid 2

   for ( hyb = 0; hyb < 3; hyb++ ) {
      for ( apv = 0; apv < 5; apv++ ) {
         regName.str("");
         regName << "FilterLd" << dec << hyb << apv;
         addRegister(new Register(regName.str(), 0x01300000 + (hyb*5+apv)*2));

         regName.str("");
         regName << "FilterWr" << dec << hyb << apv;
         addRegister(new Register(regName.str(), 0x01300001 + (hyb*5+apv)*2));
      }
   }

   // Setup variables
   addVariable(new Variable("FpgaVersion", Variable::Status));
   variables_["FpgaVersion"]->setDescription("FPGA version field");

   addVariable(new Variable("TrigCount", Variable::Status));
   variables_["TrigCount"]->setDescription("Trig Count Value");
   variables_["TrigCount"]->setComp(0,1,0,"");

   addVariable(new Variable("AdcEnable", Variable::Configuration));
   variables_["AdcEnable"]->setDescription("Enable ADC channel mask");
   variables_["AdcEnable"]->setPerInstance(true);

   addVariable(new Variable("AdcClkInvert", Variable::Configuration));
   variables_["AdcClkInvert"]->setDescription("Invert ADC clock");
   variables_["AdcClkInvert"]->setTrueFalse();
   variables_["AdcClkInvert"]->setPerInstance(true);

   addVariable(new Variable("TisClkEn",Variable::Configuration));
   variables_["TisClkEn"]->setDescription("TIS Clock Enable");
   variables_["TisClkEn"]->setTrueFalse();

   addVariable(new Variable("TholdEnable",Variable::Configuration));
   variables_["TholdEnable"]->setDescription("Threshold Enable");
   variables_["TholdEnable"]->setTrueFalse();

   addVariable(new Variable("ApvSyncDetect", Variable::Status));
   variables_["ApvSyncDetect"]->setDescription("APV sync detect status. 8-bit mask.\n"
                                               "One bit per channe. 0=APV0, 1=APV1...");

   addVariable(new Variable("ApvTrigType", Variable::Configuration));
   variables_["ApvTrigType"]->setDescription("Set APV trigger type. Double or single.");
   vector<string> trigTypes;
   trigTypes.resize(6);
   trigTypes[0] = "Test";
   trigTypes[1] = "SingleTrig";
   trigTypes[2] = "DoubleTrig";
   trigTypes[3] = "SingleCalib";
   trigTypes[4] = "DoubleCalib";
   trigTypes[5] = "SyncRead";
   variables_["ApvTrigType"]->setEnums(trigTypes);

   addVariable(new Variable("ApvTrigSource", Variable::Configuration));
   variables_["ApvTrigSource"]->setDescription("Set trigger source.");
   vector<string> trigSources;
   trigSources.resize(4);
   trigSources[0] = "None";
   trigSources[1] = "Internal";
   trigSources[2] = "Software";
   trigSources[3] = "External";
   variables_["ApvTrigSource"]->setEnums(trigSources);

   addVariable(new Variable("ApvTrigGenPause", Variable::Configuration));
   variables_["ApvTrigGenPause"]->setDescription("Set internal trigger generation period.");
   variables_["ApvTrigGenPause"]->setComp(0,0.025,0,"uS");
   variables_["ApvTrigGenPause"]->setRange(0,99999999);

   addVariable(new Variable("ClockSelect", Variable::Configuration));
   variables_["ClockSelect"]->setDescription("Selects between internally and externally generated APV and ADC Clk.");
   vector<string> clkSel;
   clkSel.resize(2);
   clkSel[0] = "Internal";
   clkSel[1] = "External";
   variables_["ClockSelect"]->setEnums(clkSel);

   addVariable(new Variable("DataBypass", Variable::Configuration));
   variables_["DataBypass"]->setDescription("Data bypass");
   variables_["DataBypass"]->setTrueFalse();

   addVariable(new Variable("FiltEnable", Variable::Configuration));
   variables_["FiltEnable"]->setDescription("FIR FIlter Enable");
   variables_["FiltEnable"]->setTrueFalse();

   addVariable(new Variable("TempPollPer", Variable::Configuration));
   variables_["TempPollPer"]->setDescription("Temp Polling Period.");
   variables_["TempPollPer"]->setRange(0,0x7FFFFFFF);
   variables_["TempPollPer"]->setComp(0,0.008,0,"uS");

   addVariable(new Variable("CalDelay", Variable::Configuration));
   variables_["CalDelay"]->setDescription("Cal to trig delay.");
   variables_["CalDelay"]->setComp(0,25,0,"nS");
   variables_["CalDelay"]->setRange(0,65535);

   addVariable(new Variable("InputDelayA", Variable::Configuration));
   variables_["InputDelayA"]->setDescription("ADC Channel 0 input delay.");
   variables_["InputDelayA"]->setRange(0,63);
   variables_["InputDelayA"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayB", Variable::Configuration));
   variables_["InputDelayB"]->setDescription("ADC Channel 1 input delay.");
   variables_["InputDelayB"]->setRange(0,63);
   variables_["InputDelayB"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayC", Variable::Configuration));
   variables_["InputDelayC"]->setDescription("ADC Channel 3 input delay.");
   variables_["InputDelayC"]->setRange(0,63);
   variables_["InputDelayC"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayD", Variable::Configuration));
   variables_["InputDelayD"]->setDescription("ADC Channel 4 input delay.");
   variables_["InputDelayD"]->setRange(0,63);
   variables_["InputDelayD"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayE", Variable::Configuration));
   variables_["InputDelayE"]->setDescription("ADC Channel 5 input delay.");
   variables_["InputDelayE"]->setRange(0,63);
   variables_["InputDelayE"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayF", Variable::Configuration));
   variables_["InputDelayF"]->setDescription("ADC Channel 6 input delay.");
   variables_["InputDelayF"]->setRange(0,63);
   variables_["InputDelayF"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayG", Variable::Configuration));
   variables_["InputDelayG"]->setDescription("ADC Channel 7 input delay.");
   variables_["InputDelayG"]->setRange(0,63);
   variables_["InputDelayG"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayH", Variable::Configuration));
   variables_["InputDelayH"]->setDescription("ADC Channel 8 input delay.");
   variables_["InputDelayH"]->setRange(0,63);
   variables_["InputDelayH"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayI", Variable::Configuration));
   variables_["InputDelayI"]->setDescription("ADC Channel 9 input delay.");
   variables_["InputDelayI"]->setRange(0,63);
   variables_["InputDelayI"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayJ", Variable::Configuration));
   variables_["InputDelayJ"]->setDescription("ADC Channel 10 input delay.");
   variables_["InputDelayJ"]->setRange(0,63);
   variables_["InputDelayJ"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayK", Variable::Configuration));
   variables_["InputDelayK"]->setDescription("ADC Channel 11 input delay.");
   variables_["InputDelayK"]->setRange(0,63);
   variables_["InputDelayK"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayL", Variable::Configuration));
   variables_["InputDelayL"]->setDescription("ADC Channel 12 input delay.");
   variables_["InputDelayL"]->setRange(0,63);
   variables_["InputDelayL"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayM", Variable::Configuration));
   variables_["InputDelayM"]->setDescription("ADC Channel 13 input delay.");
   variables_["InputDelayM"]->setRange(0,63);
   variables_["InputDelayM"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayN", Variable::Configuration));
   variables_["InputDelayN"]->setDescription("ADC Channel 14 input delay.");
   variables_["InputDelayN"]->setRange(0,63);
   variables_["InputDelayN"]->setComp(0,78,0,"pS");

   addVariable(new Variable("InputDelayO", Variable::Configuration));
   variables_["InputDelayO"]->setDescription("ADC Channel 15 input delay.");
   variables_["InputDelayO"]->setRange(0,63);
   variables_["InputDelayO"]->setComp(0,78,0,"pS");

   addVariable(new Variable("FrameDelayA", Variable::Configuration));
   variables_["FrameDelayA"]->setDescription("ADC Frame 0 input delay.");
   variables_["FrameDelayA"]->setRange(0,63);
   variables_["FrameDelayA"]->setComp(0,78,0,"pS");

   addVariable(new Variable("FrameDelayB", Variable::Configuration));
   variables_["FrameDelayB"]->setDescription("ADC Frame 1 input delay.");
   variables_["FrameDelayB"]->setRange(0,63);
   variables_["FrameDelayB"]->setComp(0,78,0,"pS");

   addVariable(new Variable("FramePolA", Variable::Status));
   variables_["FramePolA"]->setDescription("ADC Frame 0 polarity.");

   addVariable(new Variable("FramePolB", Variable::Status));
   variables_["FramePolB"]->setDescription("ADC Frame 1 polarity.");

   addVariable(new Variable("AdcClkDelay", Variable::Configuration));
   variables_["AdcClkDelay"]->setDescription("ADC Output Clock Delay.");
   variables_["AdcClkDelay"]->setRange(0,63);
   variables_["AdcClkDelay"]->setComp(0,78,0,"pS");
   variables_["AdcClkDelay"]->setPerInstance(true);

   addVariable(new Variable("Temp_0_0", Variable::Status));
   variables_["Temp_0_0"]->setDescription("Hybrid 0, Temp 0");

   addVariable(new Variable("Temp_0_1", Variable::Status));
   variables_["Temp_0_1"]->setDescription("Hybrid 0, Temp 1");

   addVariable(new Variable("Temp_0_2", Variable::Status));
   variables_["Temp_0_2"]->setDescription("Hybrid 0, Temp 2");

   addVariable(new Variable("Temp_0_3", Variable::Status));
   variables_["Temp_0_3"]->setDescription("Hybrid 0, Temp 3");

   addVariable(new Variable("Temp_1_0", Variable::Status));
   variables_["Temp_1_0"]->setDescription("Hybrid 1, Temp 0");

   addVariable(new Variable("Temp_1_1", Variable::Status));
   variables_["Temp_1_1"]->setDescription("Hybrid 1, Temp 1");

   addVariable(new Variable("Temp_1_2", Variable::Status));
   variables_["Temp_1_2"]->setDescription("Hybrid 1, Temp 2");

   addVariable(new Variable("Temp_1_3", Variable::Status));
   variables_["Temp_1_3"]->setDescription("Hybrid 1, Temp 3");

   addVariable(new Variable("Temp_2_0", Variable::Status));
   variables_["Temp_2_0"]->setDescription("Hybrid 2, Temp 0");

   addVariable(new Variable("Temp_2_1", Variable::Status));
   variables_["Temp_2_1"]->setDescription("Hybrid 2, Temp 1");

   addVariable(new Variable("Temp_2_2", Variable::Status));
   variables_["Temp_2_2"]->setDescription("Hybrid 2, Temp 2");

   addVariable(new Variable("Temp_2_3", Variable::Status));
   variables_["Temp_2_3"]->setDescription("Hybrid 2, Temp 3");

   addVariable(new Variable("SyncDataA", Variable::Status));
   variables_["SyncDataA"]->setDescription("Sync Data A");

   addVariable(new Variable("SyncDataB", Variable::Status));
   variables_["SyncDataB"]->setDescription("Sync Data B");

   addVariable(new Variable("SyncDataC", Variable::Status));
   variables_["SyncDataC"]->setDescription("Sync Data C");

   addVariable(new Variable("SyncDataD", Variable::Status));
   variables_["SyncDataD"]->setDescription("Sync Data D");

   addVariable(new Variable("SyncDataE", Variable::Status));
   variables_["SyncDataE"]->setDescription("Sync Data E");

   addVariable(new Variable("SyncDataF", Variable::Status));
   variables_["SyncDataF"]->setDescription("Sync Data F");

   addVariable(new Variable("SyncDataG", Variable::Status));
   variables_["SyncDataG"]->setDescription("Sync Data G");

   addVariable(new Variable("SyncDataH", Variable::Status));
   variables_["SyncDataH"]->setDescription("Sync Data H");

   addVariable(new Variable("SyncDataI", Variable::Status));
   variables_["SyncDataI"]->setDescription("Sync Data I");

   addVariable(new Variable("SyncDataJ", Variable::Status));
   variables_["SyncDataJ"]->setDescription("Sync Data J");

   addVariable(new Variable("SyncDataK", Variable::Status));
   variables_["SyncDataK"]->setDescription("Sync Data K");

   addVariable(new Variable("SyncDataL", Variable::Status));
   variables_["SyncDataL"]->setDescription("Sync Data L");

   addVariable(new Variable("SyncDataM", Variable::Status));
   variables_["SyncDataM"]->setDescription("Sync Data M");

   addVariable(new Variable("SyncDataN", Variable::Status));
   variables_["SyncDataN"]->setDescription("Sync Data N");

   addVariable(new Variable("SyncDataO", Variable::Status));
   variables_["SyncDataO"]->setDescription("Sync Data O");

   addVariable(new Variable("NewRegisters", Variable::Configuration));
   variables_["NewRegisters"]->setDescription("Enable new registers");
   variables_["NewRegisters"]->setTrueFalse();

   addVariable(new Variable("TrigEdgeNeg", Variable::Configuration));
   variables_["TrigEdgeNeg"]->setDescription("Trigger Edge Negative");
   variables_["TrigEdgeNeg"]->setTrueFalse();

   // Commands
   addCommand(new Command("ApvSWTrig",0x0));
   commands_["ApvSWTrig"]->setDescription("Generate APV software trigger + calibration.");

   addCommand(new Command("MasterReset"));
   commands_["MasterReset"]->setDescription("Send master FPGA reset.\n"
                                            "Wait a few moments following reset generation before\n"
                                            "issuing addition commands or configuration read/writes");

   addCommand(new Command("Apv25Reset"));
   commands_["Apv25Reset"]->setDescription("Send APV25 RESET101.");

   addCommand(new Command("ApvClkRst"));
   commands_["ApvClkRst"]->setDescription("APV Clock Reset.");

   addCommand(new Command("Apv25HardReset"));
   commands_["Apv25HardReset"]->setDescription("Assert reset line to APV25s.");

   addCommand(new Command("TrigCntRst"));
   commands_["TrigCntRst"]->setDescription("Trigger count reset");

   // Add sub-devices
   addDevice(new Hybrid(destination,0x01100000, 0,this));
   addDevice(new Hybrid(destination,0x01110000, 1,this));
   addDevice(new Hybrid(destination,0x01120000, 2,this));
   addDevice(new Ad9252(destination,0x01140000, 0,this));
   addDevice(new Ad9252(destination,0x01150000, 1,this));
}

// Deconstructor
CntrlFpga::~CntrlFpga ( ) { }

// Method to process a command
void CntrlFpga::command ( string name, string arg) {

   // Command is local
   if ( name == "MasterReset" ) {
      REGISTER_LOCK
      registers_["MasterReset"]->set(0x1);
      writeRegister(registers_["MasterReset"],true,false);
      REGISTER_UNLOCK
   }
   else if ( name == "Apv25Reset" ) {
      REGISTER_LOCK
      registers_["Apv25Reset"]->set(0x1);
      writeRegister(registers_["Apv25Reset"],true);
      REGISTER_UNLOCK
   }
   else if ( name == "ApvClkRst" ) {
      REGISTER_LOCK
      registers_["ApvClkRst"]->set(0x1);
      writeRegister(registers_["ApvClkRst"],true);
      registers_["ApvClkRst"]->set(0x0);
      writeRegister(registers_["ApvClkRst"],true);
      REGISTER_UNLOCK
   }
   else if ( name == "Apv25HardReset" ) {
      REGISTER_LOCK
      registers_["ApvHardReset"]->set(0x1);
      writeRegister(registers_["ApvHardReset"],true);
      REGISTER_UNLOCK
   }
   else if ( name == "TrigCntRst" ) {
      REGISTER_LOCK
      registers_["TrigCntRst"]->set(0x1);
      writeRegister(registers_["TrigCntRst"],true);
      REGISTER_UNLOCK
   }
   else Device::command(name, arg);
}

// Method to read temperature data
void CntrlFpga::readTemps ( ) {
   uint         hybrid;
   uint         temp;
   stringstream name;
   stringstream txt;
   uint         val;
   uint         tmp;

   readRegister(registers_["TempData"]);

   for (hybrid=0; hybrid < 3; hybrid++) {
      for (temp=0; temp < 4; temp++) {
         name.str("");
         name << "Temp_" << dec << hybrid;
         name << "_" << dec << temp;

         tmp = registers_["TempData"]->getIndex(hybrid*2+(temp/2));

         if ( (temp % 2) == 0 ) val = (tmp & 0xFFF);
         else val = ((tmp >> 16) & 0xFFF);

         txt.str("");
         txt << tempTable_[val] << " C (";
         txt << "0x" << hex << setw(3) << setfill('0') << val << ")";
         variables_[name.str()]->set(txt.str());
      }
   }

   readRegister(registers_["ApvSyncStatus"]);
   variables_["ApvSyncDetect"]->setInt(registers_["ApvSyncStatus"]->get(0,0x7FFF));
   variables_["FramePolA"]->setInt(registers_["ApvSyncStatus"]->get(16,0x1));
   variables_["FramePolB"]->setInt(registers_["ApvSyncStatus"]->get(17,0x1));
}

// Method to read status registers and update variables
void CntrlFpga::readStatus ( ) {
   REGISTER_LOCK
   uint         tmp;
   uint         val;
   stringstream txt;

   // Read status
   readRegister(registers_["TrigCount"]);
   variables_["TrigCount"]->setInt(registers_["TrigCount"]->get());

   readRegister(registers_["Version"]);
   variables_["FpgaVersion"]->setInt(registers_["Version"]->get());

   readTemps();

   if ( variables_["NewRegisters"]->getInt() ) readRegister(registers_["SyncData"]);

   tmp = registers_["SyncData"]->getIndex(0);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(0);
   txt << " " << dec << val;
   variables_["SyncDataA"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(1);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(1);
   txt << " " << dec << val;
   variables_["SyncDataB"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(2);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(2);
   txt << " " << dec << val;
   variables_["SyncDataC"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(3);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(3);
   txt << " " << dec << val;
   variables_["SyncDataD"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(4);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(4);
   txt << " " << dec << val;
   variables_["SyncDataE"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(5);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(5);
   txt << " " << dec << val;
   variables_["SyncDataF"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(6);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(6);
   txt << " " << dec << val;
   variables_["SyncDataG"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(7);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(7);
   txt << " " << dec << val;
   variables_["SyncDataH"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(8);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(8);
   txt << " " << dec << val;
   variables_["SyncDataI"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(9);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(9);
   txt << " " << dec << val;
   variables_["SyncDataJ"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(10);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(10);
   txt << " " << dec << val;
   variables_["SyncDataK"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(11);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(11);
   txt << " " << dec << val;
   variables_["SyncDataL"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(12);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(12);
   txt << " " << dec << val;
   variables_["SyncDataM"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(13);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(13);
   txt << " " << dec << val;
   variables_["SyncDataN"]->set(txt.str());

   tmp = registers_["SyncData"]->getIndex(14);
   val = (((tmp >> 16) & 0xFFFF) - (tmp & 0xFFFF)) / 8;
   txt.str("");
   txt << "0x" << hex << setw(8) << setfill('0') << registers_["SyncData"]->getIndex(14);
   txt << " " << dec << val;
   variables_["SyncDataO"]->set(txt.str());

   // Sub devices
   Device::readStatus();
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void CntrlFpga::readConfig ( ) {
   REGISTER_LOCK

   // Read config
   readRegister(registers_["AdcChanEn"]);
   variables_["AdcEnable"]->setInt(registers_["AdcChanEn"]->get(0,0x7FFF));
   variables_["AdcClkInvert"]->setInt(registers_["AdcChanEn"]->get(16,0x1));

   readRegister(registers_["TisClkEn"]);
   variables_["TisClkEn"]->setInt(registers_["TisClkEn"]->get(0,0x1));

   readRegister(registers_["ApvTrigSrcType"]);
   variables_["ApvTrigType"]->setInt(registers_["ApvTrigSrcType"]->get(8,0xFF));
   variables_["ApvTrigSource"]->setInt(registers_["ApvTrigSrcType"]->get(0,0xFF));

   readRegister(registers_["ApvTrigGenPause"]);
   variables_["ApvTrigGenPause"]->setInt(registers_["ApvTrigGenPause"]->get(0,0xFFFFFFFF));

   readRegister(registers_["ClockSelect"]);
   variables_["ClockSelect"]->setInt(registers_["ClockSelect"]->get(0,0x1));
   variables_["DataBypass"]->setInt(registers_["ClockSelect"]->get(1,0x1));
   variables_["FiltEnable"]->setInt(registers_["ClockSelect"]->get(2,0x1));

   readRegister(registers_["CalDelay"]);
   variables_["CalDelay"]->setInt(registers_["CalDelay"]->get(0,0xFFFF));

   readRegister(registers_["TempPollPer"]);
   variables_["TempPollPer"]->setInt(registers_["TempPollPer"]->get());

   readRegister(registers_["TrigEdgeNeg"]);
   variables_["TrigEdgeNeg"]->setInt(registers_["TrigEdgeNeg"]->get());

   readRegister(registers_["InputDelay"]);
   variables_["InputDelayA"]->setInt(registers_["InputDelay"]->getIndex(0));
   variables_["InputDelayB"]->setInt(registers_["InputDelay"]->getIndex(1));
   variables_["InputDelayC"]->setInt(registers_["InputDelay"]->getIndex(2));
   variables_["InputDelayD"]->setInt(registers_["InputDelay"]->getIndex(3));
   variables_["InputDelayE"]->setInt(registers_["InputDelay"]->getIndex(4));
   variables_["InputDelayF"]->setInt(registers_["InputDelay"]->getIndex(5));
   variables_["InputDelayG"]->setInt(registers_["InputDelay"]->getIndex(6));
   variables_["InputDelayH"]->setInt(registers_["InputDelay"]->getIndex(7));
   variables_["InputDelayI"]->setInt(registers_["InputDelay"]->getIndex(8));
   variables_["InputDelayJ"]->setInt(registers_["InputDelay"]->getIndex(9));
   variables_["InputDelayK"]->setInt(registers_["InputDelay"]->getIndex(10));
   variables_["InputDelayL"]->setInt(registers_["InputDelay"]->getIndex(11));
   variables_["InputDelayM"]->setInt(registers_["InputDelay"]->getIndex(12));
   variables_["InputDelayN"]->setInt(registers_["InputDelay"]->getIndex(13));
   variables_["InputDelayO"]->setInt(registers_["InputDelay"]->getIndex(14));

   readRegister(registers_["FrameDelayA"]);
   variables_["FrameDelayA"]->setInt(registers_["FrameDelayA"]->get(0,0x3F));

   readRegister(registers_["FrameDelayB"]);
   variables_["FrameDelayB"]->setInt(registers_["FrameDelayB"]->get(0,0x3F));

   readRegister(registers_["AdcClkDelay"]);
   variables_["AdcClkDelay"]->setInt(registers_["AdcClkDelay"]->get(0,0x3F));

   readRegister(registers_["TholdEnable"]);
   variables_["TholdEnable"]->setInt(registers_["TholdEnable"]->get(0,0x1));

   // Sub devices
   Device::readConfig();
   REGISTER_UNLOCK
}

// Coefficant order lookup. Pass order index, returns coef to load at that position
uint CntrlFpga::coefOrder  ( uint  idxIn) {
   switch (idxIn) {
      case 0:  return(0); break;
      case 1:  return(5); break;
      case 2:  return(1); break;
      case 3:  return(6); break;
      case 4:  return(2); break;
      case 5:  return(7); break;
      case 6:  return(3); break;
      case 7:  return(8); break;
      case 8:  return(4); break;
      case 9:  return(9); break;
      default: return(0); break;
   }
}

// Method to write configuration registers
void CntrlFpga::writeConfig ( bool force ) {
   stringstream regName;
   uint hyb, apv, coef;
   uint coefVal;

   REGISTER_LOCK

   registers_["ClockSelect"]->set(variables_["ClockSelect"]->getInt(),0,0x1);
   registers_["ClockSelect"]->set(variables_["DataBypass"]->getInt(),1,0x1);
   registers_["ClockSelect"]->set(variables_["FiltEnable"]->getInt(),2,0x1);
   writeRegister(registers_["ClockSelect"],force);

   registers_["AdcChanEn"]->set(variables_["AdcEnable"]->getInt(),0,0x7FFF);
   registers_["AdcChanEn"]->set(variables_["AdcClkInvert"]->getInt(),16,0x1);
   writeRegister(registers_["AdcChanEn"],force);

   registers_["TisClkEn"]->set(variables_["TisClkEn"]->getInt(),0,0x1);
   writeRegister(registers_["TisClkEn"],force);

   registers_["ApvTrigGenPause"]->set(variables_["ApvTrigGenPause"]->getInt(),0,0xFFFFFFFF);
   writeRegister(registers_["ApvTrigGenPause"],force);

   registers_["ApvTrigSrcType"]->set(variables_["ApvTrigType"]->getInt(),8,0xFF);
   registers_["ApvTrigSrcType"]->set(variables_["ApvTrigSource"]->getInt(),0,0xFF);
   writeRegister(registers_["ApvTrigSrcType"],force);

   registers_["TempPollPer"]->set(variables_["TempPollPer"]->getInt());
   writeRegister(registers_["TempPollPer"],force);

   registers_["CalDelay"]->set(variables_["CalDelay"]->getInt(),0,0xFFFF);
   writeRegister(registers_["CalDelay"],force);

   registers_["InputDelay"]->setIndex(0,variables_["InputDelayA"]->getInt());
   registers_["InputDelay"]->setIndex(1,variables_["InputDelayB"]->getInt());
   registers_["InputDelay"]->setIndex(2,variables_["InputDelayC"]->getInt());
   registers_["InputDelay"]->setIndex(3,variables_["InputDelayD"]->getInt());
   registers_["InputDelay"]->setIndex(4,variables_["InputDelayE"]->getInt());
   registers_["InputDelay"]->setIndex(5,variables_["InputDelayF"]->getInt());
   registers_["InputDelay"]->setIndex(6,variables_["InputDelayG"]->getInt());
   registers_["InputDelay"]->setIndex(7,variables_["InputDelayH"]->getInt());
   registers_["InputDelay"]->setIndex(8,variables_["InputDelayI"]->getInt());
   registers_["InputDelay"]->setIndex(9,variables_["InputDelayJ"]->getInt());
   registers_["InputDelay"]->setIndex(10,variables_["InputDelayK"]->getInt());
   registers_["InputDelay"]->setIndex(11,variables_["InputDelayL"]->getInt());
   registers_["InputDelay"]->setIndex(12,variables_["InputDelayM"]->getInt());
   registers_["InputDelay"]->setIndex(13,variables_["InputDelayN"]->getInt());
   registers_["InputDelay"]->setIndex(14,variables_["InputDelayO"]->getInt());
   writeRegister(registers_["InputDelay"],force);

   registers_["FrameDelayA"]->set(variables_["FrameDelayA"]->getInt(),0,0x3F);
   writeRegister(registers_["FrameDelayA"],force);

   registers_["FrameDelayB"]->set(variables_["FrameDelayB"]->getInt(),0,0x3F);
   writeRegister(registers_["FrameDelayB"],force);

   registers_["AdcClkDelay"]->set(variables_["AdcClkDelay"]->getInt(),0,0x3F);
   writeRegister(registers_["AdcClkDelay"],force);

   registers_["TholdEnable"]->set(variables_["TholdEnable"]->getInt(),0,0x1);
   writeRegister(registers_["TholdEnable"],force);

   registers_["TrigEdgeNeg"]->set(variables_["TrigEdgeNeg"]->getInt(),0,0x1);
   writeRegister(registers_["TrigEdgeNeg"],force);







/*

   //if ( index_ != 0 ) {
      for (hyb=0; hyb < 3; hyb++) {
         for (apv=0; apv < 5; apv++) {
            for (chan=0; chan < 128; chan++) {
               thold_->threshData[index_][hyb][apv][chan] = 0x0370;
            }
         }
      }
   //}
*/

   // Threshold data
   if ( thold_ != NULL ) {

      if ( force || memcmp(registers_["ThresholdA"]->data(),thold_->threshData[index_][0],640*4) != 0 ) {
         memcpy(registers_["ThresholdA"]->data(),thold_->threshData[index_][0],640*4);
         writeRegister(registers_["ThresholdA"],true);
      }
      if ( force || memcmp(registers_["ThresholdB"]->data(),thold_->threshData[index_][1],640*4) != 0 ) {
         memcpy(registers_["ThresholdB"]->data(),thold_->threshData[index_][1],640*4);
         writeRegister(registers_["ThresholdB"],true);
      }
      if ( force || memcmp(registers_["ThresholdC"]->data(),thold_->threshData[index_][2],640*4) != 0 ) {
         memcpy(registers_["ThresholdC"]->data(),thold_->threshData[index_][2],640*4);
         writeRegister(registers_["ThresholdC"],true);
      }
   }
   else {
      memset(registers_["ThresholdA"]->data(),0,640*4);
      memset(registers_["ThresholdB"]->data(),0,640*4);
      memset(registers_["ThresholdC"]->data(),0,640*4);
      if (force ) { 
         writeRegister(registers_["ThresholdA"],true);
         writeRegister(registers_["ThresholdB"],true);
         writeRegister(registers_["ThresholdC"],true);
      }
   }

   // debug_ = true;

   // Filter data when force = true
   if ( filt_ != NULL && force ) {
      for ( hyb=0; hyb < 3; hyb++) {
         for ( apv=0; apv<5; apv++) {
         
            // Start reload 
            regName.str("");
            regName << "FilterLd" << dec << hyb << apv;
            writeRegister(registers_[regName.str()],true);

            // Each coef, scale factor = 2, total bits = 16, fractional bits = 14, signed
            for ( coef = 0; coef < 10; coef++ ) {
               coefVal = (unsigned int)(((filt_->filterData[index_][hyb][apv][coefOrder(coef)]) / 2.0) * (double)pow(2,15));

               regName.str("");
               regName << "FilterWr" << dec << hyb << apv;
               registers_[regName.str()]->set(coefVal);
               writeRegister(registers_[regName.str()],true);
            }
         }
      }
   }
   // debug_ = false;

   // Sub devices
   Device::writeConfig(force);
   REGISTER_UNLOCK
}

// Verify hardware state of configuration
void CntrlFpga::verifyConfig ( ) {
   REGISTER_LOCK

   verifyRegister(registers_["AdcChanEn"]);
   verifyRegister(registers_["TisClkEn"]);
   verifyRegister(registers_["ApvTrigSrcType"]);
   verifyRegister(registers_["ApvTrigGenPause"]);
   verifyRegister(registers_["CalDelay"]);
   verifyRegister(registers_["ClockSelect"]);
   verifyRegister(registers_["TempPollPer"]);
   verifyRegister(registers_["InputDelay"]);
   verifyRegister(registers_["FrameDelayA"]);
   verifyRegister(registers_["FrameDelayB"]);
   verifyRegister(registers_["AdcClkDelay"]);
   verifyRegister(registers_["TholdEnable"]);
   verifyRegister(registers_["ThresholdA"]);
   verifyRegister(registers_["ThresholdB"]);
   verifyRegister(registers_["ThresholdC"]);
   verifyRegister(registers_["TrigEdgeNeg"]);

   Device::verifyConfig();
   REGISTER_UNLOCK
}

//! Set Threshold data pointer
void CntrlFpga::setThreshold (Threshold *thold) {
   thold_ = thold;
}

//! Set Filter data pointer
void CntrlFpga::setFilter (Filter *filt) {
   filt_ = filt;
}
