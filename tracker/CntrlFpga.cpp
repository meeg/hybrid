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
using namespace std;

// Constructor
CntrlFpga::CntrlFpga ( uint destination, uint index, Device *parent ) : 
                     Device(destination,0,"cntrlFpga",index,parent) {

   // Description
   desc_ = "Control FPGA Object.";

   // Create Registers: name, address
   addRegister(new Register("Version",         0x01000000));
   addRegister(new Register("MasterReset",     0x01000001));
   addRegister(new Register("ScratchPad",      0x01000002));
   addRegister(new Register("AdcChanEn",       0x01000003));
   addRegister(new Register("Apv25Reset",      0x01000004));
   addRegister(new Register("ApvClkRst",       0x01000005));
   addRegister(new Register("TisClkEn",        0x01000006));
   addRegister(new Register("Apv25Reset",      0x01000004));
   addRegister(new Register("ApvSyncStatus",   0x01000008));
   addRegister(new Register("ApvHardReset",    0x01000009));
   addRegister(new Register("TholdEnable",     0x0100000A));
   addRegister(new Register("ApvTrigGenPause", 0x0100000B));
   addRegister(new Register("ApvTrigSrcType",  0x0100000C));
   addRegister(new Register("ClockSelect",     0x0100000E));
   addRegister(new Register("GetTemp",         0x01000016));
   addRegister(new Register("CalDelay",        0x01000017));
   addRegister(new Register("InputDelayA",     0x01000020));
   addRegister(new Register("InputDelayB",     0x01000021));
   addRegister(new Register("InputDelayC",     0x01000022));
   addRegister(new Register("InputDelayD",     0x01000023));
   addRegister(new Register("InputDelayE",     0x01000024));
   addRegister(new Register("InputDelayF",     0x01000025));
   addRegister(new Register("InputDelayG",     0x01000026));
   addRegister(new Register("InputDelayH",     0x01000027));
   addRegister(new Register("InputDelayI",     0x01000028));
   addRegister(new Register("InputDelayJ",     0x01000029));
   addRegister(new Register("InputDelayK",     0x0100002a));
   addRegister(new Register("InputDelayL",     0x0100002b));
   addRegister(new Register("InputDelayM",     0x0100002c));
   addRegister(new Register("InputDelayN",     0x0100002d));
   addRegister(new Register("InputDelayO",     0x0100002e));
   addRegister(new Register("FrameDelayA",     0x01000030));
   addRegister(new Register("FrameDelayB",     0x01000031));
   addRegister(new Register("AdcClkDelay",     0x01000032));

   addRegister(new Register("ThresholdA",      0x01200000, 640)); // Hybrid 0
   addRegister(new Register("ThresholdB",      0x01200400, 640)); // Hybrid 1
   addRegister(new Register("ThresholdC",      0x01200800, 640)); // Hybrid 2

   // Init thresholds for testing. Will result in sample 3,4,5 matching
   // for channels 126 & 127
   for (uint fpga=0; fpga < Threshold::FpgaCount; fpga++) {
      for (uint hyb=0; hyb < Threshold::HybridCount; hyb++) {
         for (uint apv=0; apv < Threshold::ApvCount; apv++) {
            for (uint chan=0; chan < Threshold::ChanCount; chan++) {
               thold_.threshData[fpga][hyb][apv][chan] = 0x0000037d;
            }
         }
      }
   }
   memcpy(registers_["ThresholdA"]->data(),thold_.threshData[index][0],640*4);
   memcpy(registers_["ThresholdB"]->data(),thold_.threshData[index][1],640*4);
   memcpy(registers_["ThresholdC"]->data(),thold_.threshData[index][2],640*4);

   // Setup variables
   addVariable(new Variable("FpgaVersion", Variable::Status));
   variables_["FpgaVersion"]->setDescription("FPGA version field");

   addVariable(new Variable("ScratchPad", Variable::Configuration));
   variables_["ScratchPad"]->setDescription("Scratchpad for testing");

   addVariable(new Variable("Adc00Enable", Variable::Configuration));
   variables_["Adc00Enable"]->setDescription("Enable ADC channel 0");
   variables_["Adc00Enable"]->setTrueFalse();

   addVariable(new Variable("Adc01Enable", Variable::Configuration));
   variables_["Adc01Enable"]->setDescription("Enable ADC channel 1");
   variables_["Adc01Enable"]->setTrueFalse();

   addVariable(new Variable("Adc02Enable", Variable::Configuration));
   variables_["Adc02Enable"]->setDescription("Enable ADC channel 2");
   variables_["Adc02Enable"]->setTrueFalse();

   addVariable(new Variable("Adc03Enable", Variable::Configuration));
   variables_["Adc03Enable"]->setDescription("Enable ADC channel 3");
   variables_["Adc03Enable"]->setTrueFalse();

   addVariable(new Variable("Adc04Enable", Variable::Configuration));
   variables_["Adc04Enable"]->setDescription("Enable ADC channel 4");
   variables_["Adc04Enable"]->setTrueFalse();

   addVariable(new Variable("Adc05Enable", Variable::Configuration));
   variables_["Adc05Enable"]->setDescription("Enable ADC channel 5");
   variables_["Adc05Enable"]->setTrueFalse();

   addVariable(new Variable("Adc06Enable", Variable::Configuration));
   variables_["Adc06Enable"]->setDescription("Enable ADC channel 6");
   variables_["Adc06Enable"]->setTrueFalse();

   addVariable(new Variable("Adc07Enable", Variable::Configuration));
   variables_["Adc07Enable"]->setDescription("Enable ADC channel 7");
   variables_["Adc07Enable"]->setTrueFalse();

   addVariable(new Variable("Adc08Enable", Variable::Configuration));
   variables_["Adc08Enable"]->setDescription("Enable ADC channel 8");
   variables_["Adc08Enable"]->setTrueFalse();

   addVariable(new Variable("Adc09Enable", Variable::Configuration));
   variables_["Adc09Enable"]->setDescription("Enable ADC channel 9");
   variables_["Adc09Enable"]->setTrueFalse();

   addVariable(new Variable("Adc10Enable", Variable::Configuration));
   variables_["Adc10Enable"]->setDescription("Enable ADC channel 10");
   variables_["Adc10Enable"]->setTrueFalse();

   addVariable(new Variable("Adc11Enable", Variable::Configuration));
   variables_["Adc11Enable"]->setDescription("Enable ADC channel 11");
   variables_["Adc11Enable"]->setTrueFalse();

   addVariable(new Variable("Adc12Enable", Variable::Configuration));
   variables_["Adc12Enable"]->setDescription("Enable ADC channel 12");
   variables_["Adc12Enable"]->setTrueFalse();

   addVariable(new Variable("Adc13Enable", Variable::Configuration));
   variables_["Adc13Enable"]->setDescription("Enable ADC channel 13");
   variables_["Adc13Enable"]->setTrueFalse();

   addVariable(new Variable("Adc14Enable", Variable::Configuration));
   variables_["Adc14Enable"]->setDescription("Enable ADC channel 14");
   variables_["Adc14Enable"]->setTrueFalse();

   addVariable(new Variable("AdcClkInvert", Variable::Configuration));
   variables_["AdcClkInvert"]->setDescription("Invert ADC clock");
   variables_["AdcClkInvert"]->setTrueFalse();

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
   trigTypes.resize(5);
   trigTypes[0] = "Test";
   trigTypes[1] = "SingleTrig";
   trigTypes[2] = "DoubleTrig";
   trigTypes[3] = "SingleCalib";
   trigTypes[4] = "DoubleCalib";
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

   addVariable(new Variable("GetTemp", Variable::Configuration));
   variables_["GetTemp"]->setDescription("Enables getting temperature.");
   variables_["GetTemp"]->setTrueFalse();

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
   else Device::command(name, arg);
}

// Method to read status registers and update variables
void CntrlFpga::readStatus ( ) {
   REGISTER_LOCK

   // Read status
   readRegister(registers_["Version"]);
   variables_["FpgaVersion"]->setInt(registers_["Version"]->get());

   readRegister(registers_["ApvSyncStatus"]);
   variables_["ApvSyncDetect"]->setInt(registers_["ApvSyncStatus"]->get(0,0x7FFF));
   variables_["FramePolA"]->setInt(registers_["ApvSyncStatus"]->get(16,0x1));
   variables_["FramePolB"]->setInt(registers_["ApvSyncStatus"]->get(17,0x1));

   // Sub devices
   Device::readStatus();
   REGISTER_UNLOCK
}

// Method to read configuration registers and update variables
void CntrlFpga::readConfig ( ) {
   REGISTER_LOCK

   // Read config
   readRegister(registers_["ScratchPad"]);
   variables_["ScratchPad"]->setInt(registers_["ScratchPad"]->get());

   readRegister(registers_["AdcChanEn"]);
   variables_["Adc00Enable"]->setInt(registers_["AdcChanEn"]->get(0,0x1));
   variables_["Adc01Enable"]->setInt(registers_["AdcChanEn"]->get(1,0x1));
   variables_["Adc02Enable"]->setInt(registers_["AdcChanEn"]->get(2,0x1));
   variables_["Adc03Enable"]->setInt(registers_["AdcChanEn"]->get(3,0x1));
   variables_["Adc04Enable"]->setInt(registers_["AdcChanEn"]->get(4,0x1));
   variables_["Adc05Enable"]->setInt(registers_["AdcChanEn"]->get(5,0x1));
   variables_["Adc06Enable"]->setInt(registers_["AdcChanEn"]->get(6,0x1));
   variables_["Adc07Enable"]->setInt(registers_["AdcChanEn"]->get(7,0x1));
   variables_["Adc08Enable"]->setInt(registers_["AdcChanEn"]->get(8,0x1));
   variables_["Adc09Enable"]->setInt(registers_["AdcChanEn"]->get(9,0x1));
   variables_["Adc10Enable"]->setInt(registers_["AdcChanEn"]->get(10,0x1));
   variables_["Adc11Enable"]->setInt(registers_["AdcChanEn"]->get(11,0x1));
   variables_["Adc12Enable"]->setInt(registers_["AdcChanEn"]->get(12,0x1));
   variables_["Adc13Enable"]->setInt(registers_["AdcChanEn"]->get(13,0x1));
   variables_["Adc14Enable"]->setInt(registers_["AdcChanEn"]->get(14,0x1));
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

   readRegister(registers_["CalDelay"]);
   variables_["CalDelay"]->setInt(registers_["CalDelay"]->get(0,0xFFFF));

   readRegister(registers_["InputDelayA"]);
   variables_["InputDelayA"]->setInt(registers_["InputDelayA"]->get(0,0x3F));

   readRegister(registers_["InputDelayB"]);
   variables_["InputDelayB"]->setInt(registers_["InputDelayB"]->get(0,0x3F));

   readRegister(registers_["InputDelayC"]);
   variables_["InputDelayC"]->setInt(registers_["InputDelayC"]->get(0,0x3F));

   readRegister(registers_["InputDelayD"]);
   variables_["InputDelayD"]->setInt(registers_["InputDelayD"]->get(0,0x3F));

   readRegister(registers_["InputDelayE"]);
   variables_["InputDelayE"]->setInt(registers_["InputDelayE"]->get(0,0x3F));

   readRegister(registers_["InputDelayF"]);
   variables_["InputDelayF"]->setInt(registers_["InputDelayF"]->get(0,0x3F));

   readRegister(registers_["InputDelayG"]);
   variables_["InputDelayG"]->setInt(registers_["InputDelayG"]->get(0,0x3F));

   readRegister(registers_["InputDelayH"]);
   variables_["InputDelayH"]->setInt(registers_["InputDelayH"]->get(0,0x3F));

   readRegister(registers_["InputDelayI"]);
   variables_["InputDelayI"]->setInt(registers_["InputDelayI"]->get(0,0x3F));

   readRegister(registers_["InputDelayJ"]);
   variables_["InputDelayJ"]->setInt(registers_["InputDelayJ"]->get(0,0x3F));

   readRegister(registers_["InputDelayK"]);
   variables_["InputDelayK"]->setInt(registers_["InputDelayK"]->get(0,0x3F));

   readRegister(registers_["InputDelayL"]);
   variables_["InputDelayL"]->setInt(registers_["InputDelayL"]->get(0,0x3F));

   readRegister(registers_["InputDelayM"]);
   variables_["InputDelayM"]->setInt(registers_["InputDelayM"]->get(0,0x3F));

   readRegister(registers_["InputDelayN"]);
   variables_["InputDelayN"]->setInt(registers_["InputDelayN"]->get(0,0x3F));

   readRegister(registers_["InputDelayO"]);
   variables_["InputDelayO"]->setInt(registers_["InputDelayO"]->get(0,0x3F));

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

// Method to write configuration registers
void CntrlFpga::writeConfig ( bool force ) {
   REGISTER_LOCK

   // Write config
   registers_["ScratchPad"]->set(variables_["ScratchPad"]->getInt());
   writeRegister(registers_["ScratchPad"],force);

   registers_["ClockSelect"]->set(variables_["ClockSelect"]->getInt(),0,0x1);
   writeRegister(registers_["ClockSelect"],force);

   registers_["AdcChanEn"]->set(variables_["Adc00Enable"]->getInt(),0,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc01Enable"]->getInt(),1,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc02Enable"]->getInt(),2,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc03Enable"]->getInt(),3,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc04Enable"]->getInt(),4,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc05Enable"]->getInt(),5,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc06Enable"]->getInt(),6,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc07Enable"]->getInt(),7,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc08Enable"]->getInt(),8,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc09Enable"]->getInt(),9,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc10Enable"]->getInt(),10,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc11Enable"]->getInt(),11,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc12Enable"]->getInt(),12,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc13Enable"]->getInt(),13,0x1);
   registers_["AdcChanEn"]->set(variables_["Adc14Enable"]->getInt(),14,0x1);
   registers_["AdcChanEn"]->set(variables_["AdcClkInvert"]->getInt(),16,0x1);
   writeRegister(registers_["AdcChanEn"],force);

   registers_["TisClkEn"]->set(variables_["TisClkEn"]->getInt(),0,0x1);
   writeRegister(registers_["TisClkEn"],force);

   registers_["ApvTrigGenPause"]->set(variables_["ApvTrigGenPause"]->getInt(),0,0xFFFFFFFF);
   writeRegister(registers_["ApvTrigGenPause"],force);

   registers_["ApvTrigSrcType"]->set(variables_["ApvTrigType"]->getInt(),8,0xFF);
   registers_["ApvTrigSrcType"]->set(variables_["ApvTrigSource"]->getInt(),0,0xFF);
   writeRegister(registers_["ApvTrigSrcType"],force);

   registers_["GetTemp"]->set(variables_["GetTemp"]->getInt(),0,0x1);
   writeRegister(registers_["GetTemp"],force);

   registers_["CalDelay"]->set(variables_["CalDelay"]->getInt(),0,0xFFFF);
   writeRegister(registers_["CalDelay"],force);

   registers_["InputDelayA"]->set(variables_["InputDelayA"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayA"],force);

   registers_["InputDelayB"]->set(variables_["InputDelayB"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayB"],force);

   registers_["InputDelayC"]->set(variables_["InputDelayC"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayC"],force);

   registers_["InputDelayD"]->set(variables_["InputDelayD"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayD"],force);

   registers_["InputDelayE"]->set(variables_["InputDelayE"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayE"],force);

   registers_["InputDelayF"]->set(variables_["InputDelayF"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayF"],force);

   registers_["InputDelayG"]->set(variables_["InputDelayG"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayG"],force);

   registers_["InputDelayH"]->set(variables_["InputDelayH"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayH"],force);

   registers_["InputDelayI"]->set(variables_["InputDelayI"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayI"],force);

   registers_["InputDelayJ"]->set(variables_["InputDelayJ"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayJ"],force);

   registers_["InputDelayK"]->set(variables_["InputDelayK"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayK"],force);

   registers_["InputDelayL"]->set(variables_["InputDelayL"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayL"],force);

   registers_["InputDelayM"]->set(variables_["InputDelayM"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayM"],force);

   registers_["InputDelayN"]->set(variables_["InputDelayN"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayN"],force);

   registers_["InputDelayO"]->set(variables_["InputDelayO"]->getInt(),0,0x3F);
   writeRegister(registers_["InputDelayO"],force);

   registers_["FrameDelayA"]->set(variables_["FrameDelayA"]->getInt(),0,0x3F);
   writeRegister(registers_["FrameDelayA"],force);

   registers_["FrameDelayB"]->set(variables_["FrameDelayB"]->getInt(),0,0x3F);
   writeRegister(registers_["FrameDelayB"],force);

   registers_["AdcClkDelay"]->set(variables_["AdcClkDelay"]->getInt(),0,0x3F);
   writeRegister(registers_["AdcClkDelay"],force);

   registers_["TholdEnable"]->set(variables_["TholdEnable"]->getInt(),0,0x1);
   writeRegister(registers_["TholdEnable"],force);

   // Threshold data
   writeRegister(registers_["ThresholdA"],force);
   writeRegister(registers_["ThresholdB"],force);
   writeRegister(registers_["ThresholdC"],force);

   // Sub devices
   Device::writeConfig(force);
   REGISTER_UNLOCK
}

// Verify hardware state of configuration
void CntrlFpga::verifyConfig ( ) {
   REGISTER_LOCK

   verifyRegister(registers_["ScratchPad"]);
   verifyRegister(registers_["AdcChanEn"]);
   verifyRegister(registers_["TisClkEn"]);
   verifyRegister(registers_["ApvTrigSrcType"]);
   verifyRegister(registers_["ApvTrigGenPause"]);
   verifyRegister(registers_["CalDelay"]);
   verifyRegister(registers_["ClockSelect"]);
   verifyRegister(registers_["GetTemp"]);
   verifyRegister(registers_["InputDelayA"]);
   verifyRegister(registers_["InputDelayB"]);
   verifyRegister(registers_["InputDelayC"]);
   verifyRegister(registers_["InputDelayD"]);
   verifyRegister(registers_["InputDelayE"]);
   verifyRegister(registers_["InputDelayF"]);
   verifyRegister(registers_["InputDelayG"]);
   verifyRegister(registers_["InputDelayH"]);
   verifyRegister(registers_["InputDelayI"]);
   verifyRegister(registers_["InputDelayJ"]);
   verifyRegister(registers_["InputDelayK"]);
   verifyRegister(registers_["InputDelayL"]);
   verifyRegister(registers_["InputDelayM"]);
   verifyRegister(registers_["InputDelayN"]);
   verifyRegister(registers_["InputDelayO"]);
   verifyRegister(registers_["FrameDelayA"]);
   verifyRegister(registers_["FrameDelayB"]);
   verifyRegister(registers_["AdcClkDelay"]);
   verifyRegister(registers_["TholdEnable"]);
   verifyRegister(registers_["ThresholdA"]);
   verifyRegister(registers_["ThresholdB"]);
   verifyRegister(registers_["ThresholdC"]);

   Device::verifyConfig();
   REGISTER_UNLOCK
}

