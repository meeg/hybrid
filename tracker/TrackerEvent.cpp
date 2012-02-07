//-----------------------------------------------------------------------------
// File          : TrackerEvent.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/26/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Event Container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/26/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "TrackerEvent.h"
using namespace std;

void TrackerEvent::update() { }

// Constructor
TrackerEvent::TrackerEvent () : Data() {
   double       temp;
   double       tk;
   double       res;
   double       volt;
   unsigned int idx;

   // Fill temperature lookup table
   temp = minTemp_;
   while ( temp < maxTemp_ ) {
      tk = k0_ + temp;
      res = t25_ * exp(coeffA_+(coeffB_/tk)+(coeffC_/(tk*tk))+(coeffD_/(tk*tk*tk)));      
      volt = (res*vmax_)/(rdiv_+res);
      idx = (uint)((volt / vref_) * (double)(adcCnt_-1));
      if ( idx < adcCnt_ ) tempTable_[idx] = temp; 
      temp += incTemp_;
   }
}

// Deconstructor
TrackerEvent::~TrackerEvent () {
}

// Get sample size value from header.
uint TrackerEvent::sampleSize ( ) {
   return((data_[0]>>8)&0xF);
}

// Get FpgaAddress value from header.
uint TrackerEvent::fpgaAddress ( ) {
   return(data_[1]);
}

// Get sequence count from header.
uint TrackerEvent::sequence ( ) {
   return(data_[2]);
}

// Get trigger value from header.
uint TrackerEvent::trigger ( ) {
   return(data_[3]);
}

// Get temperature values from header.
double TrackerEvent::temperature ( uint index ) {
   switch (index) {
      case 0: return(tempTable_[(data_[4]&0x3FFF)]);
      case 1: return(tempTable_[((data_[4]>>16)&0x3FFF)]);
      case 2: return(tempTable_[(data_[5]&0x3FFF)]);
      case 3: return(tempTable_[((data_[5]>>16)&0x3FFF)]);
      default: return(0.0);
   }
}

// Get sample count
uint TrackerEvent::count ( ) {
   return((size_-(headSize_ + tailSize_))/sampleSize_);
}

// Get sample at index
TrackerSample *TrackerEvent::sample (uint index) {
   if ( index >= count() ) return(NULL);
   else {
      sample_.setData(&(data_[headSize_+(index*sampleSize_)]));
      return(&sample_);
   }
}

// Get sample at index
TrackerSample *TrackerEvent::sampleCopy (uint index) {
   TrackerSample *tmp;

   if ( index >= count() ) return(NULL);
   else {
      tmp = new TrackerSample (&(data_[headSize_+(index*sampleSize_)]));
      return(tmp);
   }
}

