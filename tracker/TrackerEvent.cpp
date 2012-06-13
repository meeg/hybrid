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
// 02/14/2012: Updates to match FPGA. Added hooks for future TI frames.
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
      //res = t25_ * exp(coeffA_+(coeffB_/tk)+(coeffC_/(tk*tk))+(coeffD_/(tk*tk*tk)));      
      res = constA_ * exp(beta_/tk);
      volt = (res*vmax_)/(rdiv_+res);
      idx = (uint)((volt / vref_) * (double)(adcCnt_-1));
      if ( idx < adcCnt_ ) tempTable_[idx] = temp; 
      temp += incTemp_;
   }
}

// Deconstructor
TrackerEvent::~TrackerEvent () {
}

// Get TI flag from header
bool TrackerEvent::isTiFrame ( ) {
   return((data_[0] & 0x80000000) != 0);
}

// Get FpgaAddress value from header.
uint TrackerEvent::fpgaAddress ( ) {
   return(data_[0] & 0xFFFF);
}

// Get sequence count from header.
uint TrackerEvent::sequence ( ) {
   return(data_[1]);
}

// Get trigger block from header.
uint * TrackerEvent::tiData ( ) {
   return(&(data_[2]));
}

// Get temperature values from header.
double TrackerEvent::temperature ( uint index ) {
   if ( isTiFrame () ) return(0.0);
   else switch (index) {
      case  0: return(tempTable_[(data_[2]&0xFFF)]);
      case  1: return(tempTable_[((data_[2]>>16)&0xFFF)]);
      case  2: return(tempTable_[(data_[3]&0xFFF)]);
      case  3: return(tempTable_[((data_[3]>>16)&0xFFF)]);
      case  4: return(tempTable_[(data_[4]&0xFFF)]);
      case  5: return(tempTable_[((data_[4]>>16)&0xFFF)]);
      case  6: return(tempTable_[(data_[5]&0xFFF)]);
      case  7: return(tempTable_[((data_[5]>>16)&0xFFF)]);
      case  8: return(tempTable_[(data_[6]&0xFFF)]);
      case  9: return(tempTable_[((data_[6]>>16)&0xFFF)]);
      case 10: return(tempTable_[(data_[7]&0xFFF)]);
      case 11: return(tempTable_[((data_[7]>>16)&0xFFF)]);
      default: return(0.0);
   }
}

// Get sample count
uint TrackerEvent::count ( ) {
   if ( isTiFrame () ) return(0);
   else return((size_-(headSize_ + tailSize_))/sampleSize_);
}

// Get sample at index
TrackerSample *TrackerEvent::sample (uint index) {
   if ( isTiFrame () ) return(NULL);
   else if ( index >= count() ) return(NULL);
   else {
      sample_.setData(&(data_[headSize_+(index*sampleSize_)]));
         // Improper frame size
	     if ( ((size_ - (headSize_ + tailSize_)) % sampleSize_) != 0 ) {
	           data_  = NULL;
	                 size_  = 0;
	                       cout<<"Improper frame size ... "<<endl;
	                             return(false);
	                                }
      return(&sample_);
   }
}

// Get sample at index
TrackerSample *TrackerEvent::sampleCopy (uint index) {
   TrackerSample *tmp;

   if ( isTiFrame () ) return(NULL);
   else if ( index >= count() ) return(NULL);
   else {
      tmp = new TrackerSample (&(data_[headSize_+(index*sampleSize_)]));
      return(tmp);
   }
}

