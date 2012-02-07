//-----------------------------------------------------------------------------
// File          : Hybrid.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
//-----------------------------------------------------------------------------
// Description :
// Hybrid container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __HYBRID_H__
#define __HYBRID_H__

#include <Device.h>
using namespace std;

//! Class to contain APV25 
class Hybrid : public Device {

   public:

      //! Constructor
      /*! 
       * \param destination Device destination
       * \param baseAddress Device base address
       * \param index       Device index
       * \param parent      Parent device
      */
      Hybrid ( uint destination, uint baseAddress, uint index, Device *parent );

      //! Deconstructor
      ~Hybrid ( );

};
#endif
