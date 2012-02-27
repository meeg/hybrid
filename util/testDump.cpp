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
   uint          chan;
   uint          samp;
   uint          val;
   uint          sub;
   bool          hit;

   // Check args
   if ( argc != 2 ) {
      cout << "Usage: readExample filename" << endl;
      return(1);
   }

   for (chan=0; chan < 128; chan++) {
      for (samp=0; samp < 6; samp++) {
         val  = chan & 0xFF;
         val |= (samp << 8) & 0xF00;

         //if ( val < 0x0500 ) sub = 0;
         //else sub = val - 0x0500;
         //if ( val < 0x0070 ) sub = 0;
         //else sub = val - 0x0070;
         sub = val;

         //if ( sub > 0x0070 ) hit = true;
         if ( sub > 0x057d ) hit = true;
         else hit = false;

         if ( hit ) {
            cout << "Samp=" << dec << samp << ", Chan=" << dec << chan
                 << ", val=0x" << hex << val << ", Sub=0x" << hex << sub
                 << ", hit=" << hit << endl;
         }
      }
   }


   // Attempt to open data file
   if ( ! dataRead.open(argv[1]) ) return(2);

   cout << "Fpga,Hyb,Apv,Ch" << endl;

   // Process each event
   while ( dataRead.next(&event) ) {

      if ( ! event.isTiFrame() ) {
         for (x=0; x < event.count(); x++) {
            sample = event.sample(x);

            // Show sample data
            cout << dec << sample->fpgaAddress()
                 << "," << dec << sample->hybrid()
                 << "," << dec << sample->apv()
                 << "," << dec << sample->channel() << endl;
         }
      }
   }

   // Close file
   dataRead.close();
   return(0);
}

