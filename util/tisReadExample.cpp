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
#include <iomanip>
#include <fstream>
#include <iostream>
#include <Data.h>
#include <DataRead.h>
#include <TrackerEvent.h>
using namespace std;

int main (int argc, char **argv) {
   DataRead      dataRead;
   TrackerEvent  event;
   uint          *d;
   uint          count;
   uint          fpgaAddress;
   uint          trigTime;
   uint          trigCount;
   uint          contTime;
   uint          contQuad;
   uint          contWord[4];
   uint          rxTime;
   uint          trigType;
   uint          trigQuad;
   uint          trigBusy;

   // Check args
   if ( argc != 2 ) {
      cout << "Usage: tisReadExample filename" << endl;
      return(1);
   }

   // Attempt to open data file
   if ( ! dataRead.open(argv[1]) ) return(2);

   // Process each event
   count = 0;
   while ( dataRead.next(&event) ) {
      d = event.tiData();

      trigCount   = event.sequence();
      fpgaAddress = event.fpgaAddress();

      trigType = (d[0] >> 17) & 0xFF;
      trigQuad = (d[0] >> 25) & 0x3;
      trigBusy = (d[0] >> 16) & 0x1;

      rxTime   = (d[0] << 2) & 0x3FFC;

      contWord[3] = (d[1] >> 16) & 0xFFF;
      contWord[2] = (d[1]      ) & 0xFFF;
      contWord[1] = (d[2] >> 16) & 0xFFF;
      contWord[0] = (d[2]      ) & 0xFFF;

      contQuad  = contWord[2] & 0x3;
      contTime  = contWord[2] & 0xFC;
      contTime += (contWord[3] << 8) & 0xFF00;

      trigTime   = (d[3] >> 16) & 0x0000FFFC;
      trigTime  += (d[3] << 16) & 0xFFFF0000;

      cout << "----------------------------" << endl;
      cout << "Count        = "   << dec << count << endl;
      cout << "Trig Count   = 0x" << hex << setw(8) << setfill('0') << trigCount   << endl;
      cout << "Fpga Address = 0x" << hex << setw(8) << setfill('0') << fpgaAddress << endl;
      cout << "Trig Type    = 0x" << hex << setw(4) << setfill('0') << trigType    << endl;
      cout << "Trig Quad    = 0x" << hex << setw(4) << setfill('0') << trigQuad    << endl;
      cout << "Trig Busy    = 0x" << hex << setw(4) << setfill('0') << trigBusy    << endl;
      cout << "Rx Time      = 0x" << hex << setw(4) << setfill('0') << rxTime      << endl;
      cout << "Cont Quad    = 0x" << hex << setw(4) << setfill('0') << contQuad    << endl;
      cout << "Cont Time    = 0x" << hex << setw(4) << setfill('0') << contTime    << endl;
      cout << "Cont Word 1  = 0x" << hex << setw(4) << setfill('0') << contWord[0] << endl;
      cout << "Cont Word 2  = 0x" << hex << setw(4) << setfill('0') << contWord[1] << endl;
      cout << "Cont Word 3  = 0x" << hex << setw(4) << setfill('0') << contWord[2] << endl;
      cout << "Cont Word 4  = 0x" << hex << setw(4) << setfill('0') << contWord[3] << endl;
      cout << "Mast Time    = 0x" << hex << setw(8) << setfill('0') << trigTime    << endl;
      count++;
   }

   // Dump config
   dataRead.dumpConfig();
   dataRead.dumpStatus();

   // Close file
   dataRead.close();
   return(0);
}

