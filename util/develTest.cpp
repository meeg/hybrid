//-----------------------------------------------------------------------------
// File          : develTest.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Devel board test code
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//----------------------------------------------------------------------------
#include <UdpLink.h>
#include <PgpLink.h>
#include <Tracker.h>
#include <Device.h>
#include <iomanip>
#include <fstream>
#include <iostream>
using namespace std;

int main (int argc, char **argv) {
   UdpLink      *udpLink; 
   PgpLink      *pgpLink; 
   Tracker      *tracker;
   int          timeOut = 0;
   int          detect;
   string       verify;
   int          tar;
   uint         tmp;

   if ( argc == 2 ) tar = atoi(argv[1]);
   else tar = 1;

   try {

      // Create and setup PGP link
//       pgpLink = new PgpLink;
//       pgpLink->setMaxRxTx(500000);
//       pgpLink->setDebug(true);
//       pgpLink->open("/dev/pgpcard0");
//       usleep(100);
//       cout << "Using PGP interface" << endl;

      // Create and setup PGP link
      udpLink = new UdpLink;
      udpLink->setMaxRxTx(499999);
      udpLink->setDebug(true);
      udpLink->open(8192,1,"192.168.0.16");
      usleep(100);
      cout << "Using UDP interface" << endl;

      // Setup top level device
      //tracker = new Tracker(pgpLink);
      tracker = new Tracker(udpLink);
      tracker->setDebug(true);

      // Read status registers
      cout << "Reading version register" << endl;
      tracker->device("cntrlFpga",0)->readStatus();
      cout << "Version    = " << tracker->device("cntrlFpga",0)->get("FpgaVersion") << endl;

      cout << "Reading ads7924 id register" << endl;
      tmp = tracker->device("cntrlFpga",0)->device("hybrid",0)->device("ads7924",0)->readSingle("Reset");
      cout << "Id = 0x" << hex << tmp << endl;

      //cout << "Reading ads7924 mode register" << endl;
      //tmp = tracker->device("cntrlFpga",0)->device("hybrid",0)->device("ads7924",0)->readSingle("Mode");
      //cout << "Mode = 0x" << hex << tmp << endl;

/*


      cout << "here1" << endl;
      tracker->device("cntrlFpga",0)->command("Apv25HardReset","");
      sleep(2);
      cout << "here2" << endl;

      cout << "Reading Defaults" << endl;
      tracker->command("ReadXmlFile", "defaults.xml");
      cout << "Done Reading Defaults" << endl;
      cout << "here3" << endl;

      // Read ADC status
      cout << "Reading ADC Chip ID" << endl;
      tracker->device("cntrlFpga",0)->device("ad9252",0)->readStatus();
      cout << "ADC ID    = " << tracker->device("cntrlFpga",0)->device("ad9252",0)->get("ChipId") << endl;
      cout << "ADC Grade    = " << tracker->device("cntrlFpga",0)->device("ad9252",0)->get("ChipGrade") << endl;

      // Sending RESET 101
      tracker->device("cntrlFpga",0)->command("Apv25Reset","");
      sleep(2);
 
      tracker->verifyConfig();

      do {
         tracker->device("cntrlFpga",0)->readStatus();
         detect = tracker->device("cntrlFpga",0)->getInt("ApvSyncDetect");
         cout << "Try = " << dec << ++timeOut << ", syncDetect = 0x" << hex << detect << endl;
         usleep(100);
      } while (detect == 0 && timeOut < 100);

      // Start cycle command
      if (detect > 0) {
         if ( tar > 0 ) tracker->command("OpenDataFile","doubleTrig.bin");

         tracker->setDebug(false);
         for(timeOut = 0; ( timeOut<tar || tar == 0); timeOut++) {
             tracker->device("cntrlFpga",0)->command("ApvSWTrig","");
            usleep(10000);
         }
         tracker->setDebug(true);
         sleep(1);
         tracker->command("CloseDataFile","doubleTrig.bin");
      }
*/

   } catch ( string error ) {
      cout << "Caught Error: " << endl;
      cout << error << endl;
   }
}


