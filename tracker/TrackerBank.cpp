//-----------------------------------------------------------------------------
// File          : TrackerBank.cpp
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
#include "TrackerBank.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

// Constructor
TrackerBank::TrackerBank () {
   double       temp;
   double       tk;
   double       res;
   double       volt;
   unsigned int idx;

   // Fill temperature lookup table
   temp = minTemp_;
   while ( temp < maxTemp_ ) {
      tk = k0_ + temp;
      res = constA_ * exp(beta_/tk);
      volt = (res*vmax_)/(rdiv_+res);
      idx = (uint)((volt / vref_) * (double)(adcCnt_-1));
      if ( idx < adcCnt_ ) tempTable_[idx] = temp; 
      temp += incTemp_;
   }
   data_        = NULL;
   size_        = 0;
   tag_         = 0;
   sampleCount_ = 0;
}

// Update bank data
bool TrackerBank::updateBank(uint *data, uint size, uint tag) {

   // Set data
   data_  = data;
   size_  = size;
   tag_   = tag;

   // Frame is too small
   if ( size_ < (headSize_ + tailSize_) ) {
      data_  = NULL;
      size_  = 0;
      cout<<"Frame is too small ... "<<endl;
      return(false);
   }

   // Improper frame size
   if ( ((size_ - (headSize_ + tailSize_)) % sampleSize_) != 0 ) {
      data_  = NULL;
      size_  = 0;
      cout<<"Improper frame size ... "<<endl;
      return(false);
   }
   sampleCount_=(size_-(headSize_ + tailSize_))/sampleSize_;
 
   return(true);
}

// Check for errors
uint TrackerBank::sampleErrors ( ) { 
   return(data_[size_-1]);
}

// Deconstructor
TrackerBank::~TrackerBank () {
}

// Get TI flag from header
bool TrackerBank::isTiFrame ( ) {
   return(tag_ == 7);
}

// Get FpgaAddress value from header.
uint TrackerBank::fpgaAddress ( ) {
   return(tag_);
}

// Get sequence count from header.
uint TrackerBank::sequence ( ) {
   if ( data_ == NULL ) return(0);
   else return(data_[0]);
}

// Get trigger block from header.
uint * TrackerBank::tiData ( ) {
   if ( data_ == NULL ) return(NULL);
   else return(&(data_[1]));
}

// Get temperature values from header.
double TrackerBank::temperature ( uint index ) {
   if ( data_ == NULL || isTiFrame() ) return(0.0);
   else switch (index) {
      case  0: return(tempTable_[(data_[1]&0xFFF)]);
      case  1: return(tempTable_[((data_[1]>>16)&0xFFF)]);
      case  2: return(tempTable_[(data_[2]&0xFFF)]);
      case  3: return(tempTable_[((data_[2]>>16)&0xFFF)]);
      case  4: return(tempTable_[(data_[3]&0xFFF)]);
      case  5: return(tempTable_[((data_[3]>>16)&0xFFF)]);
      case  6: return(tempTable_[(data_[4]&0xFFF)]);
      case  7: return(tempTable_[((data_[4]>>16)&0xFFF)]);
      case  8: return(tempTable_[(data_[5]&0xFFF)]);
      case  9: return(tempTable_[((data_[5]>>16)&0xFFF)]);
      case 10: return(tempTable_[(data_[6]&0xFFF)]);
      case 11: return(tempTable_[((data_[6]>>16)&0xFFF)]);
      default: return(0.0);
   }
}

// Get sample count
uint TrackerBank::count ( ) {
   if ( data_ == NULL || isTiFrame() ) return(0);
   else return(sampleCount_);
}

// Get sample at index
TrackerSample *TrackerBank::sample (uint index) {
   if ( data_ == NULL || isTiFrame () || index >= sampleCount_ ) return(NULL);
   else {
      sample_.setData(&(data_[headSize_+(index*sampleSize_)]));
      return(&sample_);
   }
}

// Get sample at index
TrackerSample *TrackerBank::sampleCopy (uint index) {
   TrackerSample *tmp;

   if ( data_ == NULL || isTiFrame () || index >= sampleCount_ ) return(NULL);
   else {
      tmp = new TrackerSample (&(data_[headSize_+(index*sampleSize_)]));
      return(tmp);
   }
}

