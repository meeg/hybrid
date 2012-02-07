//-----------------------------------------------------------------------------
// File          : Hybrid.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// Hybrid container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <Hybrid.h>
#include <Apv25.h>
#include <ApvMux.h>
#include <Device.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <Variable.h>
#include <Ads7924.h>
using namespace std;

// Constructor
Hybrid::Hybrid ( uint destination, uint baseAddress, uint index, Device *parent ) : 
                     Device(destination,baseAddress,"hybrid",index,parent) {

   // Description
   desc_ = "Heavy Photon tracker hybrid container.";

   // Add sub-devices
   addDevice(new Apv25  (destination,baseAddress + 0x00003700, 0,this));
   addDevice(new Apv25  (destination,baseAddress + 0x00003600, 1,this));
   addDevice(new Apv25  (destination,baseAddress + 0x00003500, 2,this));
   addDevice(new Apv25  (destination,baseAddress + 0x00003400, 3,this));
   addDevice(new Apv25  (destination,baseAddress + 0x00003300, 4,this));
   addDevice(new Ads7924(destination,baseAddress + 0x00004800, 0,this));

   variables_["enabled"]->set("False");
}

// Deconstructor
Hybrid::~Hybrid ( ) { }

