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
//-----------------------------------------------------------------------------

#include <Variable.h>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
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
   values_ = enums;
   value_  = values_[0];
}

// Set as true/false
void Variable::setTrueFalse ( ) {
   vector<string> trueFalse;
   trueFalse.resize(2);
   trueFalse[0] = "False";
   trueFalse[1] = "True";
   values_ = trueFalse;
   value_  = values_[0];
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

   // Lock access
   pthread_mutex_lock(&mutex_);

   value_ = value;

   // Unlock access
   pthread_mutex_unlock(&mutex_);
}

// Method to get variable value
string Variable::get ( ) {
   return(value_);
}

// Method to set variable register value
void Variable::setInt ( uint value ) {
   stringstream tmp;
   uint         x;
   tmp.str("");

   // Lock access
   pthread_mutex_lock(&mutex_);

   // Variable is an enum
   if ( values_.size() != 0 ) {

      // Valid value
      if ( value < values_.size() ) value_ = values_.at(value);

      // Invalid
      else {
         value_ = "";
         tmp << "Variable::setInt -> Name: " << name_ << endl;
         tmp << "   Invalid enum value: 0x" << hex << setw(0) << value << endl;
         tmp << "   Valid Enums: " << endl;
         for ( x=0; x < values_.size(); x++ ) 
            tmp << "      0x" << hex << setw(0) << x << " - " << values_.at(x) << endl;
         pthread_mutex_unlock(&mutex_);
         throw(tmp.str());
      }
   } 
   else {
      tmp.str("");
      tmp << "0x" << hex << setw(0) << value;
      value_ = tmp.str();
   }

   // Unlock access
   pthread_mutex_unlock(&mutex_);
}

// Method to get variable register value
uint Variable::getInt ( ) {
   stringstream tmp;
   uint         ret;
   const char   *sptr;
   char         *eptr;
   uint         x;

   // Value can't be converted to integer
   if ( value_ == "" ) return(0);

   if ( values_.size() != 0 ) {
      for (x=0; x < values_.size(); x++) {
         if (value_ == values_.at(x)) return(x);
      }
      tmp.str("");
      tmp << "Variable::setInt -> Name: " << name_ << endl;
      tmp << "   Invalid enum string: " << value_ << endl;
      tmp << "   Valid Enums: " << endl;
      for ( x=0; x < values_.size(); x++ ) 
         tmp << "      0x" << hex << setw(0) << x << " - " << values_.at(x) << endl;
      throw(tmp.str());
   }
   else {
      sptr = value_.c_str();
      ret = (uint)strtoul(sptr,&eptr,0);

      // Check for error
      if ( *eptr != '\0' || eptr == sptr ) {
         tmp.str("");
         tmp << "Variable::getInt -> Name: " << name_;
         tmp << ", Value is not an integer: " << value_ << endl;
         throw(tmp.str());
      }
      return(ret);
   }
   return(0);
}

//! Method to get variable information in xml form.
string Variable::getXmlStructure (bool hidden) {
   EnumVector::iterator   enumIter;
   stringstream           tmp;

   if ( isHidden_ && !hidden ) return(string(""));

   tmp.str("");
   tmp << "<variable>" << endl;
   tmp << "<name>" << name_ << "</name>" << endl;
   tmp << "<type>";
   switch ( type_ ) {
      case Configuration : tmp << "Configuration";  break;
      case Status        : tmp << "Status";         break;
      case Feedback      : tmp << "Feedback";       break;
      default : tmp << "Unkown"; break;
   }
   tmp << "</type>" << endl;

   // Enums
   if ( values_.size() != 0 ) {
      for ( enumIter = values_.begin(); enumIter != values_.end(); enumIter++ ) 
         tmp << "<enum>" << (*enumIter) << "</enum>" << endl;
   }

   // Computations
   if ( compValid_ ) {
      tmp << "<compA>" << compA_ << "</compA>" << endl;
      tmp << "<compB>" << compB_ << "</compB>" << endl;
      tmp << "<compC>" << compC_ << "</compC>" << endl;
      tmp << "<compUnits>" << compUnits_ << "</compUnits>" << endl;
   }

   // Range
   if ( rangeMin_ != rangeMax_ ) {
      tmp << "<min>" << dec << rangeMin_ << "</min>" << endl;
      tmp << "<max>" << dec << rangeMax_ << "</max>" << endl;
   }

   if ( desc_ != "" ) tmp << "<description>" << desc_ << "</description>" << endl;

   if ( perInstance_ ) tmp << "<perInstance/>" << endl;
   if ( isHidden_ )    tmp << "<hidden/>" << endl;

   tmp << "</variable>" << endl;
   return(tmp.str());
}

