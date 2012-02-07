//-----------------------------------------------------------------------------
// File          : CntrlFpga.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// Control FPGA container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __CNTRL_FPGA_H__
#define __CNTRL_FPGA_H__

#include <Device.h>
using namespace std;

//! Class to contain APV25 
class CntrlFpga : public Device {

   public:

      //! Constructor
      /*! 
       * \param destination Device destination
       * \param index       Device index
       * \param parent      Parent device
      */
      CntrlFpga ( uint destination, uint index, Device *parent );

      //! Deconstructor
      ~CntrlFpga ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      void command ( string name, string arg );

      //! Method to read status registers and update variables
      /*! 
       * Throws string on error.
      */
      void readStatus ( );

      //! Method to read configuration registers and update variables
      /*! 
       * Throws string on error.
      */
      void readConfig ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      void writeConfig ( bool force );

      //! Verify hardware state of configuration
      void verifyConfig ( );

};
#endif
