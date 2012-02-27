//-----------------------------------------------------------------------------
// File          : codaEmulate.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 02/18/2012
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Coda interface emulation.
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 02/18/2012: created
//----------------------------------------------------------------------------
#include <ControlCmdMem.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <evr.h>
#include <HpPciMod.h>
using namespace std;
using namespace Pds;

int main (int argc, char **argv) {
   ControlCmdMemory  *cmem;
   uint              x,y;
   uint              perSize;
   uint              perTotal;
   time_t            perTime;
   time_t            currTime;
   double            rate;
   double            drate;
   stringstream      cmd;
   int               fd;
   uint              cycles;
   HpPciRx           hpRx;
   uint              buffer[8192*16*16];
   uint              tis[8192];
   uint              rxCount;

   if ( argc != 2 ) {
      cout << "Usage: codaEmulate cycle_count (n * 16)" << endl;
      return(1);
   }
   cycles = atoi(argv[1]) * 16;

   cout << "Nice return=" << dec << nice(-5) << endl;

   fd = open("/dev/hppci0",O_RDWR);

   if ( !fd ) {
      cout << "Could not open hppci" << endl;
      return(1);
   }

   if ( controlCmdOpenAndMap(&cmem) < 0 ) {
      cout << "Failed to open shared memory" << endl;
      return(1);
   }

   time(&currTime);
   perTime  = currTime;
   perSize  = 0;
   perTotal = 0;

   // Init structure
   hpRx.model      = sizeof(uint);
   hpRx.debugLevel = 1;
   hpRx.rxMax      = 8192;
   hpRx.data       = buffer;
   hpRx.rxSize     = 0;
   hpRx.rxLane     = 0;
   hpRx.eofe       = 0;
   hpRx.fifoErr    = 0;
   hpRx.lengthErr  = 0;

   // Set defaults;
   cmd.str("");
   cmd << "<system>\n";
   cmd << "<command>\n";
   cmd << "<SetDefaults/>\n";
   cmd << "</command>\n";
   cmd << "</system>\n";
   controlCmdSend (cmem,cmd.str().c_str() );
   while ( controlCmdPending(cmem) ) usleep(1);
   sleep(1);

   // Set cycle count
   cmd.str("");
   cmd << "<system>\n";
   cmd << "<config>\n";
   cmd << "<tisFpga>\n";
   cmd << "<IntTrigCount>";
   cmd << dec << cycles;
   cmd << "</IntTrigCount>\n";
   cmd << "</tisFpga>\n";
   cmd << "</config>\n";
   cmd << "</system>\n";
   controlCmdSend (cmem,cmd.str().c_str() );
   while ( controlCmdPending(cmem) ) usleep(1);
   sleep(1);

   // Start run
   cmd.str("");
   cmd << "<system>\n";
   cmd << "<command>\n";
   cmd << "<tisFpga index=\"0\">\n";
   cmd << "<IntTrigStart></IntTrigStart>\n";
   cmd << "</tisFpga>\n";
   cmd << "</command>\n";
   cmd << "</system>\n";
   controlCmdSend (cmem,cmd.str().c_str() );
   while ( controlCmdPending(cmem) ) usleep(1);

   while (perTotal < cycles) {
      
      // Pull 16 entries
      rxCount = 0;
      for ( x=0; x < 16; x++ ) {
         hpRx.data   = &(buffer[rxCount+1]);
         hpRx.rxLane = 7;

         // Copy to event buffer
         while ( read(fd,&hpRx,sizeof(hpRx)) <= 0 ) usleep(1);
         buffer[rxCount] = hpRx.rxSize;

         // Copy to tis record
         memcpy(tis[x*6],buffer[rxCount+3],6*4);

         rxCount += (hpRx.rxSize+1);

         // Pull data entries when ready
         for (y=0; y < 7; y++) {
            hpRx.data   = &(buffer[rxCount+1]);
            hpRx.rxLane = y;

            while ( read(fd,&hpRx,sizeof(hpRx)) <= 0 ) usleep(1);
            buffer[rxCount] = hpRx.rxSize;
            rxCount += (hpRx.rxSize+1);
         }
      }
      write(fd,NULL,0);
      perTotal += 16;
      perSize  += rxCount;
   }

   time(&currTime);
   rate  = (double)perTotal / (double)(currTime - perTime);
   drate = ((double)perSize*4.0) / (double)(currTime - perTime);

   cout << "Total=" << dec << perTotal
        << ", Rate=" << rate << "hz"
        << ", Data=" << drate << "Bps" << endl;

   sleep(5);
   cout << "Done" << endl;
}

