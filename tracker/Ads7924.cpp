//-----------------------------------------------------------------------------
// File          : Ads7924.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/20/2012
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// ADS7924 AtoD
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/20/2012: created
//-----------------------------------------------------------------------------
#include <Ads7924.h>
#include <Register.h>
#include <Variable.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
Ads7924::Ads7924 ( uint destination, uint baseAddress, uint index, Device *parent ) : 
                        Device(destination,baseAddress,"ads7924",index,parent) {

   // Description
   desc_ = "ADS7924 AtoD Converter";

   // Create Registers: name, address
   addRegister(new Register("Mode",      baseAddress_ + 0x00));
   addRegister(new Register("IntCntrl",  baseAddress_ + 0x01));
   addRegister(new Register("Data0U",    baseAddress_ + 0x02));
   addRegister(new Register("Data0L",    baseAddress_ + 0x03));
   addRegister(new Register("Data1U",    baseAddress_ + 0x04));
   addRegister(new Register("Data1L",    baseAddress_ + 0x05));
   addRegister(new Register("Data2U",    baseAddress_ + 0x06));
   addRegister(new Register("Data2L",    baseAddress_ + 0x07));
   addRegister(new Register("Data3U",    baseAddress_ + 0x08));
   addRegister(new Register("Data3L",    baseAddress_ + 0x09));
   addRegister(new Register("Ulr0",      baseAddress_ + 0x0a));
   addRegister(new Register("Llr0",      baseAddress_ + 0x0b));
   addRegister(new Register("Ulr1",      baseAddress_ + 0x0c));
   addRegister(new Register("Llr1",      baseAddress_ + 0x0d));
   addRegister(new Register("Ulr2",      baseAddress_ + 0x0e));
   addRegister(new Register("Llr2",      baseAddress_ + 0x0f));
   addRegister(new Register("Ulr3",      baseAddress_ + 0x10));
   addRegister(new Register("Llr3",      baseAddress_ + 0x11));
   addRegister(new Register("IntConfig", baseAddress_ + 0x12));
   addRegister(new Register("SlpConfig", baseAddress_ + 0x13));
   addRegister(new Register("AcqConfig", baseAddress_ + 0x14));
   addRegister(new Register("PwrConfig", baseAddress_ + 0x15));
   addRegister(new Register("Reset",     baseAddress_ + 0x16));
}

// Deconstructor
Ads7924::~Ads7924 ( ) { }

void Ads7924::command ( string name, string arg ) {





}

