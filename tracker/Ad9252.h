//-----------------------------------------------------------------------------
// File          : Ad9252.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// AD9252 ADC
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __AD9252_H__
#define __AD9252_H__

#include <Device.h>
using namespace std;

//! Class to contain AD9252
class Ad9252 : public Device {

   public:

      //! Constructor
      /*! 
       * \param destination Device destination
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      Ad9252 ( uint destination, uint baseAddress, uint index, Device *parent );

      //! Deconstructor
      ~Ad9252 ( );

      //! Method to read status registers and update variables
      void readStatus ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      void writeConfig ( bool force );
};

#endif
