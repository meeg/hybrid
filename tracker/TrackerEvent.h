//-----------------------------------------------------------------------------
// File          : TrackerEvent.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/26/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Event data container.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/26/2011: created
//----------------------------------------------------------------------------
// Description :
// Event Container
// Event Data consists of the following: Z[xx:xx] = Zeros
//    Frame Size = 1 x 32-bits (32-bit dwords)
//    Header = 8 x 32-bits
//       Header[0] = Z[19:0], SampleSize[3:0], Revision[7:0]
//       Header[1] = FpgaAddress[31:0]
//       Header[2] = Sequence[31:0]
//       Header[3] = TriggerCode[31:0]
//       Header[4] = TempB[15:0], TempA[15:0]
//       Header[5] = TempD[15:0], TempC[15:0]
//       Header[6] = Z[31:0]
//       Header[7] = Z[31:0]
//
//       Samples... (See TrackerSample.h)
//
//   Tail = 1 x 32-bits
//       Should be zero
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/26/2011: created
//-----------------------------------------------------------------------------
#ifndef __TRACKER_EVENT_H__
#define __TRACKER_EVENT_H__

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include "TrackerSample.h"
#include <Data.h>
using namespace std;

//! Tracker Event Container Class
class TrackerEvent : public Data {

      // Temperature Constants
      static const double coeffA_  = -1.4141963e1;
      static const double coeffB_  =  4.4307830e3;
      static const double coeffC_  = -3.4078983e4;
      static const double coeffD_  = -8.8941929e6;
      static const double t25_     = 10000.0;
      static const double k0_      = 273.15;
      static const double vmax_    = 2.5;
      static const double vref_    = 2.5;
      static const double rdiv_    = 10000;
      static const double minTemp_ = -50;
      static const double maxTemp_ = 150;
      static const double incTemp_ = 0.01;
      static const uint   adcCnt_  = 4096;

      // Temperature lookup table
      double tempTable_[adcCnt_];

      // Frame Constants
      static const unsigned int headSize_   = 8;
      static const unsigned int tailSize_   = 1;
      static const unsigned int sampleSize_ = 4;

      // Internal sample contrainer
      TrackerSample sample_;

      // Update
      void update();

   public:

      //! Constructor
      TrackerEvent ();

      //! Deconstructor
      ~TrackerEvent ();

      //! Get sample size value from header.
      /*!
       * Returns sample size.
      */
      uint sampleSize ( );

      //! Get FpgaAddress value from header.
      /*!
       * Returns fpgaAddress
      */
      uint fpgaAddress ( );

      //! Get sequence count from header.
      /*!
       * Returns sequence count
      */
      uint sequence ( );

      //! Get trigger value from header.
      /*!
       * Returns trigger value
      */
      uint trigger ( );

      //! Get temperature values from header.
      /*!
       * Returns temperature value.
       * \param index temperature index, 0-3.
      */
      double temperature ( uint index );

      //! Get sample count
      /*!
       * Returns sample count
      */
      uint count ( );

      //! Get sample at index
      /*!
       * Returns pointer to static sample object without memory allocation.
       * Contents of returned object will change next time sample() is called.
       * \param index Sample index. 0 - count()-1.
      */
      TrackerSample *sample (uint index);

      //! Get sample at index
      /*!
       * Returns pointer to copy of sample object. A newly allocated sample object
       * is created and must be deleted after use.
       * \param index Sample index. 0 - count()-1.
      */
      TrackerSample *sampleCopy (uint index);

};

#endif
