//-----------------------------------------------------------------------------
// File          : TrackerDisplayData.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Tracker display data
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#include "TrackerDisplayData.h"
#include <iostream>
using namespace std;

// Constructor
TrackerDisplayData::TrackerDisplayData ( ) : QwtRasterData () {
   data_ = (uint *)malloc(size_*sizeof(uint));
   setInterval( Qt::XAxis, QwtInterval( xMin_, xMax_ ) );
   setInterval( Qt::YAxis, QwtInterval( yMin_, yMax_ ) );
   maxRange_ = 0;
   init();
}

// Delete
TrackerDisplayData::~TrackerDisplayData ( ) { 
   free(data_);
}

// Value
double TrackerDisplayData::value ( double x, double y ) const {
   uint posx;
   uint posy;
   uint pos;

   if ( x < xMin_ || x > xMax_ || y < yMin_ || y > yMax_ ) return(0);
   posx = (uint)x-xMin_;
   posy = (uint)y-yMin_;
   pos  = (posy * xSize_) + posx;

   return(double(data_[pos]));
}

// Fill value
void TrackerDisplayData::fill ( uint x, uint y ) {
   uint posx;
   uint posy;
   uint pos;

   if ( x < xMin_ || x > xMax_ || y < yMin_ || y > yMax_ ) return;
   posx = (uint)x-xMin_;
   posy = (uint)y-yMin_;
   pos  = (posy * xSize_) + posx;

   data_[pos]++;

   if ( data_[pos] > maxValue_ ) maxValue_ = data_[pos];

   if ( maxRange_ == 0 ) setInterval( Qt::ZAxis, QwtInterval( 0, maxValue_ ) );
   else setInterval( Qt::ZAxis, QwtInterval( 0, maxRange_ ) );
}

// Init data
void TrackerDisplayData::init ( ) {
   memset(data_,0,(size_ * sizeof(uint)));
   maxValue_ = 10;
   setInterval( Qt::ZAxis, QwtInterval( 0, maxValue_ ) );
}

void TrackerDisplayData::setMax(uint max) {
   maxRange_ = max;

   if ( maxRange_ == 0 ) setInterval( Qt::ZAxis, QwtInterval( 0, maxValue_ ) );
   else setInterval( Qt::ZAxis, QwtInterval( 0, maxRange_ ) );
}

