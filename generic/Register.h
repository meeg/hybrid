//-----------------------------------------------------------------------------
// File          : Register.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic register container.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __REGISTER_H__
#define __REGISTER_H__

#include <string>
#include <sys/types.h>
using namespace std;

//! Class to contain generic register data.
class Register {

      // Register address
      uint address_;

      // Register name
      string name_;

      // Current register value
      uint *value_;

      // Current register size
      uint size_;

      // Register stale
      bool stale_;

      // Register status
      uint status_;

   public:

      //! Constructor
      Register ( Register *reg );

      //! Constructor
      /*! 
       * \param name        Register name
       * \param address     Register address
      */
      Register ( string name, uint address );

      //! Constructor
      /*! 
       * \param name        Register name
       * \param base        Register base address
       * \param size        Register size
      */
      Register ( string name, uint base, uint size );

      //! DeConstructor
      ~Register ( );

      //! Method to get register name
      string name ();

      //! Method to get register address
      uint address ();

      void setAddress(uint address);

      //! Method to get register size
      uint size ();

      //! Method to get register data pointer
      uint *data ();

      //! Set status value
      /*! 
       * \param status Status value
      */
      void setStatus(uint status);

      //! Get status value
      uint status();

      //! Clear register stale
      void clrStale();

      //! Set register stale
      void setStale();

      //! Get register stale
      bool stale();

      //! Method to set register value
      /*!
       * Update the shadow register with the new value. Optional start
       * bit and mask to set a field within the register. Register state
       * will be set to stale.
       * \param value register value
       * \param bit start bit for field
       * \param mask mask for field
      */
      void set ( uint value, uint bit=0, uint mask=0xFFFFFFFF );

      //! Method to get register value
      /*!
       * Return the value of the register as a whole or a field within
       * the register. Optional start bit and mask to set a field within 
       * the register.
       * \param bit start bit for field
       * \param mask mask for field
      */
      uint get ( uint bit=0, uint mask=0xFFFFFFFF );

      //! Method to set register value
      /*!
       * Update the shadow register with the new value. 
       * Register state will be set to stale.
       * \param index register index
       * \param value register value
      */
      void setIndex ( uint index, uint value, uint bit=0, uint mask=0xFFFFFFFF );

      //! Method to get register value
      /*!
       * Return the value of the register as a whole or a field within
       * the register. 
       * \param index register index
      */
      uint getIndex ( uint index, uint bit=0, uint mask=0xFFFFFFFF );


};
#endif
