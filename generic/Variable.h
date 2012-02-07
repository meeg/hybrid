//-----------------------------------------------------------------------------
// File          : Variable.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic variable container.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include <string>
#include <vector>
#include <pthread.h>
#include <sys/types.h>
using namespace std;

// Local types
typedef vector<string> EnumVector;

//! Class to contain generic variable data.
class Variable {

      // Mutux variable for thread locking
      pthread_mutex_t mutex_;

   public:

      //! Variable Type Constants
      enum VariableType {
         Configuration      = 0, /*!< Variable Is Configuration */
         Status             = 1, /*!< Variable Is Status */
         Feedback           = 2  /*!< Variable Is configuration feedback */
      };

   private:

      // Variable name
      string name_;

      // Current variable value
      string value_;

      // Enum vector
      EnumVector values_;

      // Variable Type
      VariableType type_;

      // Compute constants
      bool   compValid_;
      double compA_;
      double compB_;
      double compC_;
      string compUnits_;

      // Range values
      uint rangeMin_;
      uint rangeMax_;

      // Variable description
      string desc_;

      // Variable is instance specific
      bool perInstance_;

      // Variable is hidden
      bool isHidden_;

   public:

      //! Constructor
      /*! 
       * \param name    name of variable
       * \param type    VariableType value
      */
      Variable ( string name, VariableType type );

      //! Set enum list      
      /*! 
       * \param enums Vector of enum values
      */
      void setEnums ( EnumVector enums );
      
      //! Set variable as true/false
      void setTrueFalse ( );

      //! Set computation constants
      /*! 
       * These values determine how to convert the variable 
       * integer into a readable value. The equation used is:
       * (value + compA) * compB + compC.
       * \param compA compA constant
       * \param compB compB constant
       * \param compC compC constant
       * \param compUnits Units value to add to end of computed value
      */
      void setComp ( double compA, double compB, double compC, string compUnits );

      //! Set range values
      /*! 
       * \param min   Range minimum
       * \param max   Range maximum
      */
      void setRange ( uint min, uint max );

      //! Set variable description
      /*! 
       * \param description variable description
      */
      void setDescription ( string description );

      //! Set per-instance status
      /*! 
       * This field determines if the variable is unique per-instance.
       * \param state per-instance status
      */
      void setPerInstance ( bool state );

      //! Get per-instance status
      bool perInstance ( );

      //! Set hidden status
      /*! 
       * This field determines if the variable is hidden.
       * \param state hidden status
      */
      void setHidden ( bool state );

      //! Get hidden status
      bool hidden ( );

      //! Get variable name
      string name();

      //! Get variable type
      VariableType type();

      //! Method to set variable value
      /*! 
       * \param value variable value
      */
      void set ( string value );

      //! Method to get variable value
      /*! 
       * Returns variable value
      */
      string get ( );

      //! Method to set variable integer value
      /*!
       * Throws string on error
       * \param value integer value
      */
      void setInt ( uint value );

      //! Method to get variable integer value
      /*!
       * Returns integer value 
       * Throws string on error
      */
      uint getInt ( );

      //! Method to get variable information in xml form.
      /*!
       * \param hidden include hidden variables
      */
      string getXmlStructure ( bool hidden );
};
#endif
