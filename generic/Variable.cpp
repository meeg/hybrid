//-----------------------------------------------------------------------------
// File          : Variable.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic variable container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
// 02/07/2011: Added map option
//-----------------------------------------------------------------------------

#include <Variable.h>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
using namespace std;

// Constructor
Variable::Variable ( string name, VariableType type ) {
   name_        = name;
   value_       = "";
   type_        = type;
   compValid_   = false;
   compA_       = 0;
   compB_       = 0;
   compC_       = 0;
   compUnits_   = "";
   rangeMin_    = 0;
   rangeMax_    = 0;
   desc_        = "";
   perInstance_ = (type == Status);
   isHidden_    = false;

   pthread_mutex_init(&mutex_,NULL);
}

// Set enum list      
void Variable::setEnums ( EnumVector enums ) {
   uint x;

   values_.clear();

   for ( x=0; x < enums.size(); x++ ) 
      values_.insert(pair<uint,string>(x,enums[x]));

   value_ = values_[0];
}

// Set map 
void Variable::setMap ( EnumMap map ) {
   values_ = map;

   value_ = values_.begin()->first;
}

// Set as true/false
void Variable::setTrueFalse ( ) {
   values_.clear();
   values_[0] = "False";
   values_[1] = "True";

   value_ = values_[0];
}

// Set computation constants
void Variable::setComp ( double compA, double compB, double compC, string compUnits ) {
   compValid_ = true;
   compA_     = compA;
   compB_     = compB;
   compC_     = compC;
   compUnits_ = compUnits;
}

// Set range
void Variable::setRange ( uint min, uint max ) {
   rangeMin_    = min;
   rangeMax_    = max;
}

// Set variable description
void Variable::setDescription ( string description ) {
   desc_ = description;
}

// Set per-instance status
void Variable::setPerInstance ( bool state ) {
   perInstance_ = state;
}

// Get per-instance status
bool Variable::perInstance ( ) {
   return(perInstance_);
}

// Set hidden status
void Variable::setHidden ( bool state ) {
   isHidden_ = state;
}

// Get hidden status
bool Variable::hidden () {
   return(isHidden_);
}

// Get type
Variable::VariableType Variable::type() {
   return(type_);
}

// Get name
string Variable::name() {
   return(name_);
}

// Method to set variable value
void Variable::set ( string value ) {
   pthread_mutex_lock(&mutex_);
   value_ = value;
   pthread_mutex_unlock(&mutex_);
}

// Method to get variable value
string Variable::get ( ) {
   string ret;

   pthread_mutex_lock(&mutex_);
   ret = value_;
   pthread_mutex_unlock(&mutex_);

   return(ret);
}

// Method to set variable register value
void Variable::setInt ( uint value ) {
   string       newValue;
   stringstream tmp;

   tmp.str("");
   newValue = "";

   // Variable is an enum
   if ( values_.size() != 0 ) {

      try {
         newValue = values_.at(value);
      }
      catch (const out_of_range& oor) {
         tmp << "Variable::setInt -> Name: " << name_ << endl;
         tmp << "   Invalid enum value: 0x" << hex << setw(0) << value << endl;
         throw(tmp.str());
      }
   } 
   else {
      tmp.str("");
      tmp << "0x" << hex << setw(0) << value;
      newValue = tmp.str();
   }

   pthread_mutex_lock(&mutex_);
   value_ = newValue;
   pthread_mutex_unlock(&mutex_);
}

// Method to set variable register value (displayed in decimal)
void Variable::setIntDec ( uint value ) {
   string       newValue;
   stringstream tmp;

   tmp.str("");
   newValue = "";

   // Variable is an enum
   if ( values_.size() != 0 ) {

      try {
         newValue = values_.at(value);
      }
      catch (const std::out_of_range& oor) {
         tmp << "Variable::setInt -> Name: " << name_ << endl;
         tmp << "   Invalid enum value: 0x" << hex << setw(0) << value << endl;
         throw(tmp.str());
      }
   } 
   else {
      tmp.str("");
      tmp << dec  << setw(0) << value;
      newValue = tmp.str();
   }

   pthread_mutex_lock(&mutex_);
   value_ = newValue;
   pthread_mutex_unlock(&mutex_);
}


// Method to get variable register value
uint Variable::getInt ( ) {
   stringstream      tmp;
   uint              ret;
   const char        *sptr;
   char              *eptr;
   string            lvalue;
   EnumMap::iterator enumIter;

   pthread_mutex_lock(&mutex_);
   lvalue = value_;
   pthread_mutex_unlock(&mutex_);

   // Value can't be converted to integer
   if ( lvalue == "" ) return(0);

   // Enum
   if ( values_.size() != 0 ) {

      // Find the value
      for (enumIter = values_.begin(); enumIter != values_.end(); enumIter++) {
         if ( enumIter->second == lvalue ) return(enumIter->first);
      }

      // Value was not found
      tmp.str("");
      tmp << "Variable::setInt -> Name: " << name_ << endl;
      tmp << "   Invalid enum string: " << lvalue << endl;
      throw(tmp.str());
   }
   else {
      sptr = lvalue.c_str();
      ret = (uint)strtoul(sptr,&eptr,0);

      // Check for error
      if ( *eptr != '\0' || eptr == sptr ) {
         tmp.str("");
         tmp << "Variable::getInt -> Name: " << name_;
         tmp << ", Value is not an integer: " << lvalue << endl;
         throw(tmp.str());
      }
      return(ret);
   }
   return(0);
}

//! Method to get variable information in xml form.
string Variable::getXmlStructure (bool hidden, uint level) {
   EnumMap::iterator enumIter;
   stringstream      tmp;

   if ( isHidden_ && !hidden ) return(string(""));

   tmp.str("");
   if ( level != 0 ) for (uint l=0; l < (level*3); l++) tmp << " ";
   tmp << "<variable>" << endl;
   if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "<name>" << name_ << "</name>" << endl;
   if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "<type>";
   switch ( type_ ) {
      case Configuration : tmp << "Configuration";  break;
      case Status        : tmp << "Status";         break;
      case Feedback      : tmp << "Feedback";       break;
      default : tmp << "Unkown"; break;
   }
   if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
   tmp << "</type>" << endl;

   // Enums
   if ( values_.size() != 0 ) {
      for ( enumIter = values_.begin(); enumIter != values_.end(); enumIter++ ) {
         if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
         tmp << "<enum>" << enumIter->second << "</enum>" << endl;
      }
   }

   // Computations
   if ( compValid_ ) {
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compA>" << compA_ << "</compA>" << endl;
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compB>" << compB_ << "</compB>" << endl;
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compC>" << compC_ << "</compC>" << endl;
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<compUnits>" << compUnits_ << "</compUnits>" << endl;
   }

   // Range
   if ( rangeMin_ != rangeMax_ ) {
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<min>" << dec << rangeMin_ << "</min>" << endl;
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<max>" << dec << rangeMax_ << "</max>" << endl;
   }

   if ( desc_ != "" ) {
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<description>" << desc_ << "</description>" << endl;
   }

   if ( perInstance_ ) {
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<perInstance/>" << endl;
   }

   if ( isHidden_ ) {
      if ( level != 0 ) for (uint l=0; l < ((level*3)+3); l++) tmp << " ";
      tmp << "<hidden/>" << endl;
   }

   if ( level != 0 ) for (uint l=0; l < (level*3); l++) tmp << " ";
   tmp << "</variable>" << endl;
   return(tmp.str());
}

