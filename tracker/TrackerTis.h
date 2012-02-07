//-----------------------------------------------------------------------------
// File          : TrackerTis.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon TrackerTis
//-----------------------------------------------------------------------------
// Description :
// TrackerTis Top Device
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __TRACKER_TIS_H__
#define __TRACKER_TIS_H__

#include <System.h>
using namespace std;

class CommLink;

//! Class to contain APV25 
class TrackerTis : public System {

   public:

      //! Constructor
      TrackerTis (CommLink *commLink);

      //! Deconstructor
      ~TrackerTis ( );

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

      //! Return local state machine, specific to each implementation
      string localState();

      //! Method to perform soft reset
      void softReset ( );

      //! Method to perform hard reset
      void hardReset ( );

};
#endif
