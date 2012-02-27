//-----------------------------------------------------------------------------
// File          : readExample.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 08/26/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Read data example
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 08/26/2011: created
//----------------------------------------------------------------------------
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <Data.h>
#include <DataRead.h>
using namespace std;

int main (int argc, char **argv) {
   DataRead      dataRead;
   TrackerEvent  event;
   TrackerSample *sample;
   uint          x;
   uint          y;

   // Check args
   if ( argc != 2 ) {
      cout << "Usage: readExample filename" << endl;
      return(1);
   }

   // Attempt to open data file
   if ( ! dataRead.open(argv[1]) ) return(2);

   // Process each event
   while ( dataRead.next(&event) ) {

      // Iterate through samples
      for (x=0; x < event.count(); x++) {

         // Get sample
         sample = event.sample(x);

         if ( sample->apv() != 0 && sample->apv() != 4 ) {
            for (y=0; y < 6; y++) {
               if (sample->value(y) < 7023 && sample->value(y) != 0) {
                  cout << "Found hit. Value=" << dec << sample->value(y) 
                       << ", Event=" << dec << x
                       << ", Sample=" << dec << y
                       << ", Apv=" << dec << sample->apv() << endl;
               }
            }
         }
      }

   }

   // Close file
   dataRead.close();
   return(0);
}

