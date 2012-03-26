//-----------------------------------------------------------------------------
// File          : PgpLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// PGP communications link
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __PGP_LINK_H__
#define __PGP_LINK_H__

#include <sys/types.h>
#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>
using namespace std;

//! Class to contain PGP communications link
class PgpLink : public CommLink {

   protected:

      // Device info
      string device_;
      int    fd_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      PgpLink ( );

      //! Deconstructor
      ~PgpLink ( );

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param device pgpcard device
      */
      void open ( string device );

      //! Stop threads and close link
      void close ();

};
#endif
