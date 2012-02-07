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

   // Check args
   if ( argc != 2 ) {
      cout << "Usage: readExample filename" << endl;
      return(1);
   }

   // Attempt to open data file
   if ( ! dataRead.open(argv[1]) ) return(2);

   // Process each event
   while ( dataRead.next(&event) ) {

      // Dump header values
      cout << "Header:sampleSize  = " << dec << event.sampleSize() << endl;
      cout << "Header:fpgaAddress = 0x" << hex << setw(8) << setfill('0') << event.fpgaAddress() << endl;
      cout << "Header:sequence    = 0x" << hex << setw(8) << setfill('0') << event.sequence() << endl;
      cout << "Header:trigger     = 0x" << hex << setw(8) << setfill('0') << event.trigger() << endl;
      cout << "Header:temp[0]     = " << event.temperature(0) << endl;
      cout << "Header:temp[1]     = " << event.temperature(1) << endl;
      cout << "Header:temp[2]     = " << event.temperature(2) << endl;
      cout << "Header:temp[3]     = " << event.temperature(3) << endl;
      cout << "Header:count       = " << dec << event.count() << endl;

      // Iterate through samples
      for (x=0; x < event.count(); x++) {

         // Get sample
         sample = event.sample(x);

         // Show sample data
         cout << "Sample:index       = " << dec << x << endl;
         cout << "Sample:hybrid      = " << dec << sample->hybrid() << endl;
         cout << "Sample:apv         = " << dec << sample->apv() << endl;
         cout << "Sample:channel     = " << dec << sample->channel() << " 0x" << hex << sample->channel() << endl;
         cout << "Sample:fpgaAddress = 0x" << hex << setw(8) << setfill('0') << sample->fpgaAddress() << endl;
         cout << "Sample:value[0]    = 0x" << hex << setw(4) << setfill('0') << sample->value(0) << endl;
         cout << "Sample:value[1]    = 0x" << hex << setw(4) << setfill('0') << sample->value(1) << endl;
         cout << "Sample:value[2]    = 0x" << hex << setw(4) << setfill('0') << sample->value(2) << endl;
         cout << "Sample:value[3]    = 0x" << hex << setw(4) << setfill('0') << sample->value(3) << endl;
         cout << "Sample:value[4]    = 0x" << hex << setw(4) << setfill('0') << sample->value(4) << endl;
         cout << "Sample:value[5]    = 0x" << hex << setw(4) << setfill('0') << sample->value(5) << endl;
      }

   }

   // Dump config
   dataRead.dumpConfig();
   dataRead.dumpStatus();

   // Close file
   dataRead.close();
   return(0);
}

