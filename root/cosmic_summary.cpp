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
   TCanvas         *c1, *c2, *c3, *c4, *c5;
   TH2F            *histAll;
   TH2F            *histCos;
   TH1F            *histSng[640];
   double          histMin[640];
   double          histMax[640];
   double          histCosMin;
   double          histCosMax;
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
   double          value;
   double          svalue[6];
   uint            channel;
   uint            tarChan;
   uint            tarFpga;
   uint            tarHybrid;
   uint            eventCount;
   char            name[100];
   bool            valid[640];
   uint            grCount;
   uint            scnt;
   uint            ecnt;
   bool            posSig;

   gStyle->SetOptStat(kFALSE);

   // Start X11 view
   TApplication theApp("App",NULL,NULL);

   // Root file is the first and only arg
   if ( argc != 6 ) {
      cout << "Usage: cosmic_summary fpga hybrid channel base_file data_file\n";
      return(1);
   }
   tarFpga   = atoi(argv[1]);
   tarHybrid = atoi(argv[2]);
   tarChan   = atoi(argv[3]);

   // 2d histogram
   histAll = new TH2F("Value_Hist_All","Value_Hist_All",16384,0,16384,640,0,640);
   histCos = new TH2F("Value_Hist_Cos","Value_Hist_Cos",16384,-8192,8192,640,0,640);

   for (channel=0; channel < 640; channel++) {
      sprintf(name,"%i",channel);
      histSng[channel] = new TH1F(name,name,16384,0,16384);
      histMin[channel] = 16384;
      histMax[channel] = -16384;
      valid[channel]   = false;
   }
   allMin = 16384;
   allMax = 0;
   histCosMin = 8192;
   histCosMax = -8192;

   // Attempt to open data file
   if ( ! dataRead.open(argv[4]) ) return(2);

   // Process each event
   eventCount = 0;

   while ( dataRead.next(&event) ) {
      if ( eventCount == 0 ) {
         if ( dataRead.getConfig("cntrlFpga:hybrid:apv25:PreampPolarity") == "Inverting") {
            posSig = true;
            cout << "Detected positive signals" << endl;
         }
         else {
            posSig = false;
            cout << "Detected negative signals" << endl;
         }
      }

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
         grMean[channel]  = histSng[channel]->GetFunction("gaus")->GetParameter(1);
         grSigma[channel] = histSng[channel]->GetFunction("gaus")->GetParameter(2);
         grChan[channel]  = channel;
      } else {
         grMean[channel]  = 0;
         grSigma[channel] = 0;
         grChan[channel]  = channel;
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
   mean = new TGraph(640,grChan,grMean);
   mean->Draw("a*");

   c4 = new TCanvas("c4","c4");
   c4->cd();
   sigma = new TGraph(640,grChan,grSigma);
   sigma->Draw("a*");

   // Attempt to open data file
   dataRead.close();
   if ( ! dataRead.open(argv[5]) ) return(2);

   // Process each event
   eventCount = 0;
   ecnt       = 0;

   while ( dataRead.next(&event) ) {
      if ( eventCount == 0 ) {
         if ( dataRead.getConfig("cntrlFpga:hybrid:apv25:PreampPolarity") == "Inverting") {
            posSig = true;
            cout << "Detected positive signals" << endl;
         }
         else {
            posSig = false;
            cout << "Detected negative signals" << endl;
         }
      }

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
                  scnt = 0;
                  for ( y=0; y < 6; y++ ) {
                     svalue[y] = (double)sample->value(y) - grMean[channel];
                     //if ( posSig && sample->apv() != 0 && svalue[y] > (5*grSigma[channel]) ) scnt++;
                     //if ( (!posSig) && sample->apv() != 0 && svalue[y] < (-5*grSigma[channel]) ) scnt++;
                     if ( sample->apv() != 0 && ((svalue[y] < (-5*grSigma[channel])) ||
                                                 (svalue[y] > (5*grSigma[channel])) ) ) scnt++;
                  }

                  if ( scnt > 2 ) {
                     cout << "Found hit."
                          << " V0=" << svalue[0]
                          << ", V1=" << svalue[1]
                          << ", V2=" << svalue[2]
                          << ", V3=" << svalue[3]
                          << ", V4=" << svalue[4]
                          << ", Event=" << dec << eventCount
                          << ", Count=" << dec << ecnt
                          << ", Channel=" << dec << channel
                          << ", Apv=" << dec << sample->apv() << endl;

                     ecnt++;
                     for ( y=0; y < 6; y++ ) {
                        histCos->Fill(svalue[y],channel);

                        if ( svalue[y] < histCosMin ) histCosMin = svalue[y];
                        if ( svalue[y] > histCosMax ) histCosMax = svalue[y];
                     }
                  }
               }
            }
         }
      }
      eventCount++;
   }

   c5 = new TCanvas("c5","c5");
   c5->cd();
   histCos->GetXaxis()->SetRangeUser(histCosMin,histCosMax);
   histCos->Draw("colz");

   // Start X-Windows
   theApp.Run();

   // Close file
   dataRead.close();
   return(0);
}

