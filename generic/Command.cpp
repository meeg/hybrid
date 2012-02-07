//-----------------------------------------------------------------------------
// File          : Command.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic command container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <Command.h>
#include <sstream>
using namespace std;

// Constructor
Command::Command ( string name, uint opCode ) {
   name_        = name;
   opCode_      = opCode;
   internal_    = false;
   desc_        = "";
   isHidden_    = false;
   hasArg_      = false;
}

// Constructor
Command::Command ( string name ) {
   name_        = name;
   opCode_      = 0;
   internal_    = true;
   desc_        = "";
   isHidden_    = false;
   hasArg_      = false;
}

// Set variable description
void Command::setDescription ( string description ) {
   desc_ = description;
}

// Method to get internal state
bool Command::internal () { return(internal_); }

// Method to get command name
string Command::name () { return(name_); }

// Method to get command opCode
uint Command::opCode () { return(opCode_); }

// Method to get variable information in xml form.
string Command::getXmlStructure (bool hidden) {
   stringstream           tmp;

   if ( isHidden_ && !hidden ) return(string(""));

   tmp.str("");
   tmp << "<command>" << endl;
   tmp << "<name>" << name_ << "</name>" << endl;
   if ( desc_ != "" ) tmp << "<description>" << desc_ << "</description>" << endl;
   if ( isHidden_ ) tmp << "<hidden/>" << endl;
   if ( hasArg_ ) tmp << "<hasArg/>" << endl;
   tmp << "</command>" << endl;
   return(tmp.str());
}

// Set hidden status
void Command::setHidden ( bool state ) {
   isHidden_ = state;
}

// Set has arg status
void Command::setHasArg ( bool state ) {
   hasArg_ = state;
}
