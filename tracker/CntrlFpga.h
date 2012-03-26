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
#include <Threshold.h>
using namespace std;

//! Class to contain APV25 
class CntrlFpga : public Device {

      // Temperature Constants
      static const double coeffA_  = -1.4141963e1;
      static const double coeffB_  =  4.4307830e3;
      static const double coeffC_  = -3.4078983e4;
      static const double coeffD_  = -8.8941929e6;
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

      // Threshold data
      Threshold *thold_;

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

      //! Method to read temperature data
      /*! 
       * Throws string on error.
      */
      void readTemps ( );

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

      //! Set Threshold data pointer
      void setThreshold (Threshold *thold);

};
#endif
