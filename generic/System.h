//-----------------------------------------------------------------------------
// File          : System.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic system level container.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <Device.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <libxml/tree.h>
using namespace std;

//! Class to contain generic device data.
class System : public Device {

   protected:

      // Comm link
      CommLink *commLink_;

      // Thread Routines
      static void *swRunStatic ( void *t );

      // Thread pointers
      pthread_t swRunThread_;

      // Defaults File
      string defaults_;

      // Software run thread
      virtual void swRunThread();

      // Run Variables
      bool   swRunEnable_;
      bool   swRunning_;
      uint   swRunPeriod_;
      uint   swRunCount_;
      uint   swRunProgress_;
      string swRunRetState_;
      string swRunError_;

      // Current state for polling
      bool allStatusReq_;
      bool topStatusReq_;
      bool allConfigReq_;

      // Configure status
      string configureMsg_;

      // Return data buffer
      string errorBuffer_;
      bool   errorFlag_;

      // Tracking counters
      uint   lastFileCount_;
      uint   lastDataCount_;
      time_t lastTime_;

      // Parse XML
      bool parseXml ( string input, bool force );

   public:

      //! Constructor
      /*! 
       * \param name     System name
       * \param commLink CommLink object
      */
      System ( string name, CommLink *commLink );

      //! Deconstructor
      virtual ~System ( );

      //! Get comm link
      CommLink * commLink();

      //! Method to process a command
      /*!
       * Throws string on error
       * \param name     Command name
       * \param arg      Optional arg
      */
      virtual void command ( string name, string arg );

      //! Method to set run state
      /*!
       * Set run state for the system. Default states are
       * Stopped & Running. Stopped must always be supported.
       * \param state    New run state
      */
      virtual void setRunState ( string state );

      //! Parse XML string
      /*!
       * \param xml XML string
      */
      void parseXmlString ( string input );

      //! Parse XML File
      /*!
       * \param xml XML string
      */
      void parseXmlFile ( string file );

      //! Method to perform soft reset
      virtual void softReset ( );

      //! Method to perform hard reset
      virtual void hardReset ( );

      //! Return local state machine, specific to each implementation
      virtual string localState();

      //! Poll system level status and process return messages
      string poll();

      //! Return structure string
      /*! 
       * \param hidden Set true to include hidden variables & commands
      */
      string structureString(bool hidden);

      //! Return status string
      string statusString(bool hidden);

      //! Return config string
      string configString(bool hidden);

};
#endif
