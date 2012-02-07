//-----------------------------------------------------------------------------
// File          : Ads7924.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/20/2012
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// ADS7924 AtoD Converter
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/20/2012: created
//-----------------------------------------------------------------------------
#ifndef __ADS7924_H__
#define __ADS7924_H__

#include <Device.h>
using namespace std;

//! Class to contain AD9510
class Ads7924 : public Device {

   public:

      //! Constructor
      /*! 
       * \param destination Device destination
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      Ads7924 ( uint destination, uint baseAddress, uint index, Device *parent );

      //! Deconstructor
      ~Ads7924 ( );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      virtual void command ( string name, string arg );

};

#endif
