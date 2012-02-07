//-----------------------------------------------------------------------------
// File          : Ad9510.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 01/20/2012
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// AD9210 PLL
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 01/20/2012: created
//-----------------------------------------------------------------------------
#ifndef __AD9510_H__
#define __AD9510_H__

#include <Device.h>
using namespace std;

//! Class to contain AD9510
class Ad9510 : public Device {

   public:

      //! Constructor
      /*! 
       * \param destination Device destination
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      Ad9510 ( uint destination, uint baseAddress, uint index, Device *parent );

      //! Deconstructor
      ~Ad9510 ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      void writeConfig ( bool force );
};

#endif
