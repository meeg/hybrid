//-----------------------------------------------------------------------------
// File          : TrackerSample.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/26/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Sample Container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/26/2011: created
//-----------------------------------------------------------------------------
#include <string.h>
#include "TrackerSample.h"
using namespace std;

// Constructor for static pointer
TrackerSample::TrackerSample () {
   data_ = ldata_;
}

// Constructor with copy
TrackerSample::TrackerSample ( uint *data ) {
   data_ = ldata_;
   memcpy(ldata_,data,16);
}

// Set data pointer.
void TrackerSample::setData ( uint *data ) {
   data_ = data;
}

// Get error flag
bool TrackerSample::error ( ) {
   return((data_[0]>>30)&0x1);
}

// Get threshold drop flag
bool TrackerSample::drop ( ) {
   return((data_[0]>>27)&0x1);
}

// Get hybrid index.
uint TrackerSample::hybrid ( ) {
   return((data_[0]>>28)&0x3);
}

// Get apv index.
uint TrackerSample::apv ( ) {
   return((data_[0]>>24)&0x7);
}

// Get channel index.
uint TrackerSample::channel ( ) {
   return((data_[0]>>16)&0x7F);
}

// Get FpgaAddress value from header.
uint TrackerSample::fpgaAddress ( ) {
   return(data_[0]&0xFFFF);
}

// Get adc value at index.
uint TrackerSample::value ( uint index ) {
   switch(index) {
      case 0: return(data_[1]&0x3FFF);
      case 1: return((data_[1]>>16)&0x3FFF);
      case 2: return(data_[2]&0x3FFF);
      case 3: return((data_[2]>>16)&0x3FFF);
      case 4: return(data_[3]&0x3FFF);
      case 5: return((data_[3]>>16)&0x3FFF);
      default: return(0);
   }
}

