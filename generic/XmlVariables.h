//-----------------------------------------------------------------------------
// File          : XmlVariables.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 06/05/2012
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Extract and store variables from XML string.
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __XML_VARIABLES_H__
#define __XML_VARIABLES_H__

#include <string>
#include <map>
#include <sys/types.h>
#include <libxml/tree.h>
using namespace std;

#ifdef __CINT__
#define uint unsigned int
#endif

// Define variable holder
typedef map<string,string> VariableHolder;

//! Class to contain generic register data.
class XmlVariables {

      // Strip whitespace
      string removeWhite ( string str );

      // Process level
      void xmlLevel( xmlNode *node, string curr );

      // Generate XML for a given level knowing the previous and next values
      string genXmlString ( string prevName, string currName, string currValue, string nextName );

      // Variable list
      VariableHolder vars_;

   public:

      //! Constructor
      XmlVariables ( );

      //! Deconstructor
      ~XmlVariables ( );

      //! Clear variable list
      void clear();

      //! Parse XML string
      /*! 
       * \param type Type of variable to parse, config or status
       * \param xml XML String
      */
      void parse ( string type, char *xml );

      //! Get a variable value
      /*! 
       * \param var Variable name
      */
      string get ( string var );

      //! Get a variable value as integer
      /*! 
       * \param var Variable name
      */
      uint getInt ( string var );

      //! Return variable list
      /*! 
       * \param prefix List prefix
      */
      string getList ( string prefix );

      //! Return variable list as XML
      string getXml ( );

      //! Return a single variable as XML
      string getXml (string variable);

      //! Return a string to set a variable as XML
      string setXml (string variable, string value);
};

#endif
