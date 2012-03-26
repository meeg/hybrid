//-----------------------------------------------------------------------------
// File          : UdpLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// UDP communications link
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __UDP_LINK_H__
#define __UDP_LINK_H__

#include <sys/types.h>
#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommLink.h>

using namespace std;

//! Class to contain PGP communications link
class UdpLink : public CommLink {

   protected:

      // Values used for udp version
      uint   udpCount_;
      int    *udpFd_;
      struct sockaddr_in *udpAddr_;

      // Data order fix
      bool dataOrderFix_;

      //! IO handling thread
      void ioHandler();

      //! RX handling thread
      void rxHandler();

   public:

      //! Constructor
      UdpLink ( );

      //! Deconstructor
      ~UdpLink ( );

      //! Set max receive size
      /*! 
       * \param size max receive size
      */
      void setMaxRx(uint size);

      //! Open link and start threads
      /*! 
       * Throw string on error.
       * \param port  udp port
       * \param count host count
       * \param host  udp hosts
      */
      void open ( int port, uint count, ... );

      //! Stop threads and close link
      void close ();

      //! Set data order fix flag
      /*! 
       * \param enable Enable flag
      */
      void setDataOrderFix (bool enable);

};
#endif
