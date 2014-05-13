//-----------------------------------------------------------------------------
// File          : Register.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic register container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <Register.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

//! Constructor
Register::Register ( Register *reg ) {
   address_     = reg->address_;
   name_        = reg->name_;
   stale_       = reg->stale_;
   status_      = reg->status_;
   size_        = reg->size_;

   value_       = (uint *)malloc(sizeof(uint)*size_);
   memcpy(value_,reg->value_,size_*4);
}

// Constructor
Register::Register ( string name, uint address ) {
   address_     = address;
   name_        = name;
   value_       = (uint *)malloc(sizeof(uint));
   value_[0]    = 0;
   stale_       = false;
   status_      = 0;
   size_        = 1;

}

// Constructor
Register::Register ( string name, uint address, uint size ) {
   address_     = address;
   name_        = name;
   value_       = (uint *)malloc(sizeof(uint)*size);
   stale_       = false;
   status_      = 0;
   size_        = size;

   memset(value_,0x00,(sizeof(uint)*size_));
}

// DeConstructor
Register::~Register ( ) {
   free(value_);
}

// Method to get register name
string Register::name () { return(name_); }

// Method to get register address
uint Register::address () { return(address_); }

void Register::setAddress(uint address) { address_ = address;}

// Method to get register size
uint Register::size () {
   return(size_);
}

// Method to get register data pointer
uint *Register::data () {
   return(value_);
}

// Method to set status
void Register::setStatus (uint status) {
   status_ = status;
}

// Method to get status
uint Register::status () {
   return(status_);
}

//! Clear register stale
void Register::clrStale() { stale_ = false; }

//! Set register stale
void Register::setStale() { stale_ = true; }

//! Get register stale
bool Register::stale() {
   return(stale_);
}

// Method to set register value
void Register::set ( uint value, uint bit, uint mask ) {
   setIndex(0, value, bit, mask);
}

// Method to get register value
uint Register::get ( uint bit, uint mask ) {
   return(getIndex(0, bit, mask));
}

void Register::setIndex ( uint index, uint value, uint bit, uint mask ) {
   if ( index >= size_ ) return;

   uint newVal = value_[index];
   
   newVal &= (0xFFFFFFFF ^ (mask << bit));
   newVal |= ((value & mask) << bit);

   if ( newVal != value_[index] ) {
      value_[index] = newVal;
      stale_ = true;
   }

}

uint Register::getIndex ( uint index, uint bit, uint mask ) {
   if ( index >= size_ ) return(0xFFFFFFFF);
   return((value_[index]>>bit) & mask);
}

