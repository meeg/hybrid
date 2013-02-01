//-----------------------------------------------------------------------------
// File          : Threshold.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 02/18/2012
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Threshold data container.
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 02/18/2012: created
//-----------------------------------------------------------------------------

#ifndef __THRESHOLD_H__
#define __THRESHOLD_H__

#include <vector>
#include <string>

using namespace std;

// Tracker Threshold Container Class
class Threshold {
   public:

     // Default Constructor
	 Threshold();

	 // Destructor
	 virtual ~Threshold();

	  // Constants
      static const unsigned int IdLength    = 200;
      static const unsigned int FpgaCount   = 7;
      static const unsigned int HybridCount = 3;
      static const unsigned int ApvCount    = 5;
      static const unsigned int ChanCount   = 128;

      // Sizes
      static const unsigned int FpgaSize = HybridCount * ApvCount * ChanCount;
      static const unsigned int Size     = FpgaCount * FpgaSize;

      // ID Container
      char thresholdId[IdLength];

      // Threshold container
      // Bits 31:16 = Baseline 
      // Bits 15:0  = Threshold
      unsigned int threshData[FpgaCount][HybridCount][ApvCount][ChanCount];


      //--- Methods ---//
      //---------------//
      void loadThresholdData();
      bool openFile( string );
      int  getThreshold(int, int, int, int);
      void loadDefaults();

   private:

      static const unsigned int THRESH_MASK =  0xFFFF;

	ifstream *inputFile; 
        vector<string> splitString(string, string);
};

#endif
