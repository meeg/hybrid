//-----------------------------------------------------------------------------
// File          : TrackerDisplayData.h
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
#ifndef __TRACKER_DISPLAY_DATA_H__
#define __TRACKER_DISPLAY_DATA_H__
#include <qwt_plot_spectrogram.h>
using namespace std;

class TrackerDisplayData : public QwtRasterData {

      static const uint yMin_     = 0;  
      static const uint yMax_     = (128*5)-1;  
      static const uint xMin_     = 0;  
      static const uint xMax_     = 16383;
      static const uint xSize_    = (xMax_-xMin_+1);
      static const uint ySize_    = (yMax_-yMin_+1);
      static const uint size_     = xSize_ * ySize_;
 
      uint *data_;
      uint maxValue_;
      uint maxRange_;
 
   public:

      // Window
      TrackerDisplayData ( );

      // Delete
      ~TrackerDisplayData ( );

      // Value
      virtual double value ( double x, double y ) const;

      // Fill value
      void fill ( uint x, uint y );

      // Init data
      void init ( );

      // Set max value 
      void setMax(uint max);

};

#endif
