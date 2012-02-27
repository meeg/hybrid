//-----------------------------------------------------------------------------
// File          : cal_summary.cc
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 03/03/2011
// Project       : Kpix Software Package
//-----------------------------------------------------------------------------
// Description :
// File to generate calibration summary plots.
//-----------------------------------------------------------------------------
// Copyright (c) 2009 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 03/03/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <iomanip>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TMultiGraph.h>
#include <TApplication.h>
#include <TGraph.h>
#include <TStyle.h>
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
using namespace std;

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
   TCanvas         *c1, *c2, *c3, *c4;
   TH2F            *histAll;
   TH1F            *histSng[640];
   double          histMin[640];
   double          histMax[640];
   double          allMin;
   double          allMax;
   TGraph          *mean;
   TGraph          *sigma;
   double          grChan[640];
   double          grMean[640];
   double          grSigma[640];
   DataRead        dataRead;
   TrackerEvent    event;
   TrackerSample   *sample;
   uint            x;
   uint            y;
   uint            value;
   uint            channel;
   uint            tarChan;
   uint            tarFpga;
   uint            tarHybrid;
   uint            eventCount;
   char            name[100];
   bool            valid[640];
   uint            grCount;

   gStyle->SetOptStat(kFALSE);

   // Start X11 view
   TApplication theApp("App",NULL,NULL);

   // Root file is the first and only arg
   if ( argc != 5 ) {
      cout << "Usage: hist_summary fpga hybrid channel data_file\n";
      return(1);
   }
   tarFpga   = atoi(argv[1]);
   tarHybrid = atoi(argv[2]);
   tarChan   = atoi(argv[3]);

   // 2d histogram
   histAll = new TH2F("Value_Hist_All","Value_Hist_All",16384,0,16384,640,0,640);

   for (channel=0; channel < 640; channel++) {
      sprintf(name,"%i",channel);
      histSng[channel] = new TH1F(name,name,16384,0,16384);
      histMin[channel] = 16384;
      histMax[channel] = 0;
      valid[channel]   = false;
   }
   allMin = 16384;
   allMax = 0;

   // Attempt to open data file
   if ( ! dataRead.open(argv[4]) ) return(2);

   // Process each event
   eventCount = 0;

   while ( dataRead.next(&event) ) {
      for (x=0; x < event.count(); x++) {

         // Check for matching FPGA
         if ( event.fpgaAddress() == tarFpga ) {

            // Get sample
            sample  = event.sample(x);

            // Check for matching hybrid
            if ( sample->hybrid() == tarHybrid ) {
               channel = (sample->apv() * 128) + (127-sample->channel());

               // Check for out of range channels
               if ( channel >= (5 * 128) ) {
                  cout << "Channel " << dec << channel << " out of range" << endl;
                  cout << "Apv = " << dec << sample->apv() << endl;
                  cout << "Chan = " << dec << sample->channel() << endl;
               } else {
                  valid[channel] = true;

                  // Filter APVs
                  if ( eventCount > 100 ) {
                     for ( y=0; y < 6; y++ ) {
                        value = sample->value(y);
                        histAll->Fill(value,channel);
                        histSng[channel]->Fill(value);

                        if ( value < histMin[channel] ) histMin[channel] = value;
                        if ( value > histMax[channel] ) histMax[channel] = value;
                        if ( value < allMin           ) allMin = value;
                        if ( value > allMax           ) allMax = value;
                     }
                  }
               }
            }
         }
      }
      eventCount++;
   }

   // Fit histograms
   grCount = 0;
   for(channel = 0; channel < 640; channel++) {
      if ( valid[channel] ) {
         histSng[channel]->Fit("gaus");
         histSng[channel]->GetXaxis()->SetRangeUser(histMin[channel],histMax[channel]);
         histSng[channel]->Draw();
         grMean[grCount]  = histSng[channel]->GetFunction("gaus")->GetParameter(1);
         grSigma[grCount] = histSng[channel]->GetFunction("gaus")->GetParameter(2);
         grChan[grCount]  = channel;
         grCount++;
      }
   }
   c1 = new TCanvas("c1","c1");
   c1->cd();
   histAll->Draw("colz");
   histAll->GetXaxis()->SetRangeUser(allMin,allMax);

   c2 = new TCanvas("c2","c2");
   c2->cd();
   histSng[tarChan]->Draw();

   c3 = new TCanvas("c3","c3");
   c3->cd();
   mean = new TGraph(grCount,grChan,grMean);
   mean->Draw("a*");

   c4 = new TCanvas("c4","c4");
   c4->cd();
   sigma = new TGraph(grCount,grChan,grSigma);
   sigma->Draw("a*");

   // Start X-Windows
   theApp.Run();

   // Close file
   dataRead.close();
   return(0);
}

