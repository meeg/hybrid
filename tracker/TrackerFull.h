//-----------------------------------------------------------------------------
// File          : TrackerFull.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon TrackerFull
//-----------------------------------------------------------------------------
// Description :
// TrackerFull Top Device
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __TRACKER_H__
#define __TRACKER_H__

#include <System.h>
#include <EpicsDataMem.h>
#include <Threshold.h>
using namespace std;

class CommLink;

//! Class to contain APV25 
class TrackerFull : public System {

      // Shared memory
      EpicsDataMem *tempMem_;

      // Get extracted temp value
      double getTemp(uint fpga, uint index);

      // last temp poll time
      time_t lastTemp_;

      // Threshold data
      Threshold thold_;

   public:

      //! Constructor
      TrackerFull (CommLink *commLink);

      //! Deconstructor
      ~TrackerFull ( );

      //! Method to set run state
      /*!
       * Set run state for the system. Default states are
       * Stopped & Running. Stopped must always be supported.
       * \param state    New run state
      */
      void setRunState ( string state );

      //! Method to process a command
      /*!
       * Throws string on error
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      //! Return local state, specific to each implementation
      string localState();

      //! Method to perform soft reset
      void softReset ( );

      //! Method to perform hard reset
      void hardReset ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      void writeConfig ( bool force );

};
#endif
