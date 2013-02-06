//-----------------------------------------------------------------------------
// File          : TrackerBank.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/03/2012
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Event data container for use with Evio libraries.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/03/2012: created
//----------------------------------------------------------------------------
// Description :
// Event Container
// Event Data consists of the following (after Evio header removal): Z[xx:xx] = Zeros
//    Header = 7 x 32-bits
//       Header[0] = Sequence[31:0]
//
//    The rest of the event header depends on the type.
//
//    For T = 0: (Data)
//
//       Header[1] = TempB[15:0], TempA[15:0] -- Hybrid 0
//       Header[2] = TempD[15:0], TempC[15:0] -- Hybrid 0
//       Header[3] = TempF[15:0], TempE[15:0] -- Hybrid 1
//       Header[4] = TempH[15:0], TempG[15:0] -- Hybrid 1
//       Header[5] = TempJ[15:0], TempI[15:0] -- Hybrid 2
//       Header[6] = TempL[15:0], TempK[15:0] -- Hybrid 2
//
//       Samples... (See TrackerSample.h)
//
//    For T = 1: (TI Data)
//
//       Header[1] = TBD, Waiting for clarification from JLAB
//       Header[2] = TBD, Waiting for clarification from JLAB
//       Header[3] = TBD, Waiting for clarification from JLAB
//       Header[4] = TBD, Waiting for clarification from JLAB
//       Header[5] = TBD, Waiting for clarification from JLAB
//       Header[6] = TBD, Waiting for clarification from JLAB
//
//    Tail = 1 x 32-bits
//       Should be zero. Will be non-zero if there are errors within the data.
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/03/2011: created
//-----------------------------------------------------------------------------
#ifndef __TRACKER_BANK_H__
#define __TRACKER_BANK_H__

#include <sys/types.h>
#include "TrackerSample.h"
using namespace std;

//! Tracker Bank Container Class
class TrackerBank {

      // Temperature Constants
      static const double beta_    = 3750;
      static const double constA_  = 0.03448533;
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
      static const unsigned int headSize_   = 7;
      static const unsigned int tailSize_   = 1;
      static const unsigned int sampleSize_ = 4;

      // Bank information
      uint *data_;
      uint size_;
      uint tag_;;

      // Sample count
      uint sampleCount_;

      // Internal sample contrainer
      TrackerSample sample_;

   public:

      //! Constructor
      TrackerBank ();

      //! Deconstructor
      ~TrackerBank ();

      //! Update bank data
      /*!
       * Update bank data.
       * Returns true if frame is properly formatted
       * \param data Pointer to data buffer.
       * \param size Size of data buffer in number of uint32
       * \param tag  Bank tag from EVIO header
      */
      bool updateBank(uint *data, uint size, uint tag);

      //! Return number of sample errors
      uint sampleErrors ();

      //! Is frame TI frame?
      bool isTiFrame ( );

      //! Get FpgaAddress
      /*!
       * Returns fpgaAddress
      */
      uint fpgaAddress ( );

      //! Get sequence count
      /*!
       * Returns sequence count
      */
      uint sequence ( );

      //! Get pointer to trigger block.
      /*!
       * Returns trigger block
      */
      uint * tiData ( );

      //! Get temperature values
      /*!
       * Returns temperature value.
       * \param index temperature index, 0-11.
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
