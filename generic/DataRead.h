//-----------------------------------------------------------------------------
// File          : DataRead.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Read data & configuration from disk
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __DATA_READ_H__
#define __DATA_READ_H__

#include <string>
#include <map>
#include <Data.h>
#include <sys/types.h>
#include <libxml/tree.h>
#include <iostream>
using namespace std;

// Define variable holder
typedef map<string,string> VariableHolder;

//! Class to contain generic register data.
class DataRead {

      // Strip whitespace
      string removeWhite ( string str );

      // Process xml
      void xmlParse ( uint size );

      // Process level
      void xmlLevel( xmlNode *node, string curr, bool config );

	protected:

      // File descriptor
      int fd_;

      // Config list
      VariableHolder config_;

      // Status list
      VariableHolder status_;

   public:

      //! Constructor
      DataRead ( );

      //! Deconstructor
      virtual ~DataRead ( );

      //! Open File
      /*! 
       * \param file Filename
      */
      virtual bool open ( string file );

      //! Close File
      virtual void close ( );

      //! Get next data record
      /*! 
       * Returns true on success
       * \param data Data object to store data
      */
      virtual bool next ( Data *data );

      //! Get next data record & create new data object
      /*! 
       * Returns NULL on failure
      */
      Data *next ( );

      //! Get a config value
      /*! 
       * \param var Config variable name
      */
      string getConfig ( string var );

      //! Get a status value
      /*! 
       * \param var Status variable name
      */
      string getStatus ( string var );

      //! Dump config
      void dumpConfig ( ostream &out=cout );

      //! Dump status
      void dumpStatus ( );
      
};
#endif
