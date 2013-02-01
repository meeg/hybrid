//-----------------------------------------------------------------------------
// File          : Command.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic command container.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <string>
#include <sys/types.h>
using namespace std;

//! Class to contain generic register data.
class Command {

      // Command opCode
      uint opCode_;

      // Command name
      string name_;

      // Internal state
      bool internal_;

      // Description
      string desc_;

      // Command is hidden
      bool isHidden_;

      // Command has arg
      bool hasArg_;

   public:

      //! Constructor for external commands
      /*! 
       * \param name        Command name
       * \param opCode      Command opCode
      */
      Command ( string name, uint opCode );

      //! Constructor for internal commands
      /*! 
       * \param name        Command name
      */
      Command ( string name );

      //! Set variable description
      /*! 
       * \param description variable description
      */
      void setDescription ( string description );

      //! Method to get internal state
      bool internal ();

      //! Method to get command name
      string name ();

      //! Method to get command opCode
      uint opCode ();

      //! Method to get variable information in xml form.
      /*! 
       * \param hidden Include hidden commands.
       * \param level  level for indents
      */
      string getXmlStructure ( bool hidden, uint level );

      //! Set hidden status
      /*! 
       * This field determines if the command is hidden.
       * \param state hidden status
      */
      void setHidden ( bool state );

      //! Set has arg status
      /*! 
       * This field determines if the command has an arg
       * \param state has arg status
      */
      void setHasArg ( bool state );
};
#endif
