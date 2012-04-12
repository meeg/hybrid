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
#include <fstream>
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
   TCanvas         *c1, *c2;
   TH1F            *hist[32];
   double          histMin[32];
   double          histMax[32];
   double          meanVal[32];
   double          plotX[32];
   ifstream        inFile;
   string          inLine;
   stringstream    inLineStream;
   string          inValue;
   stringstream    inValueStream;
   int             adcValue;
   uint            adcValid;
   uint            x;
   stringstream    title;
   uint            idx;
   TGraph          *plot;
   double          value;
   double          sum;

   gStyle->SetOptStat(kFALSE);

   // Start X11 view
   TApplication theApp("App",NULL,NULL);

   // Root file is the first and only arg
   if ( argc < 2 ) {
      cout << "Usage: sync_plot files.csv\n";
      return(1);
   }

   for (x=0; x < 32; x++) {
      title.str("");
      title << "sample_" << dec << x;
      hist[x] = new TH1F(title.str().c_str(),title.str().c_str(),32768,-16384,16384);
      histMin[x] = 16384;
      histMax[x] = -16384;
   }

   for (x=1; x < (uint)argc; x++) {
      inFile.open(argv[x]);

      cout << "Reading file " << argv[x] << endl;
     
      // Skip first line 
      getline(inFile,inLine);

      idx = 100;
      while ( getline(inFile,inLine) ) {
         inLineStream.clear();
         inLineStream.str(inLine);

         getline(inLineStream,inValue,'\t');
         getline(inLineStream,inValue,'\t');
         getline(inLineStream,inValue,'\t');

         getline(inLineStream,inValue,'\t');
         inValueStream.clear();
         inValueStream.str(inValue);
         inValueStream >> hex >> adcValue;

         getline(inLineStream,inValue,'\t');
         inValueStream.clear();
         inValueStream.str(inValue);
         inValueStream >> hex >> adcValid;

         if ( adcValid ) {
            cout << "Value=0x" << hex << adcValue << endl;

            if ( adcValue > 0x2000 ) idx = 0;

            if ( idx < 32 ) {
               if ( idx != 0 ) value = adcValue - 282;
               else value = adcValue;
               cout << "Fill idx=" << dec << idx << " value=0x" << hex << value << endl;
               hist[idx]->Fill(value);
               if ( value > histMin[idx] ) histMin[idx] = value;
               if ( value > histMax[idx] ) histMax[idx] = value;
            }
            idx++;
         }
      }

      inFile.close();
   }

   c1 = new TCanvas("c1","c1");
   c1->Divide(8,4,0.0125,0.0125);

   for (x=0; x < 32; x++) {
      c1->cd(x+1);
      hist[x]->Fit("gaus");
      hist[x]->GetXaxis()->SetRangeUser(histMin[x],histMax[x]);
      hist[x]->Draw();
      //meanVal[x]  = hist[x]->GetFunction("gaus")->GetParameter(1);
      //meanVal[x]  = hist[x]->GetMean() / 16383.0;
      meanVal[x]  = hist[x]->GetMean();
      plotX[x] = x;
   }

   c2 = new TCanvas("c2","c2");
   c2->cd();
   plot = new TGraph(32,plotX,meanVal);
   plot->Draw("a*");

   for (x=0; x < 32; x++) {
      cout << "Idx=" << dec << x 
           << " value=" << dec << meanVal[x] 
           << " hex=0x" << hex << (uint)meanVal[x] 
           << " adj=" << (meanVal[x] / 16383.0) << endl;
   }

   sum = 1;
   cout << "coeff: 1";
   for (x=1; x < 10; x++) {
      sum += (meanVal[x] / -16383.0);
      cout << "," << (meanVal[x] / -16383.0);
   }
   cout << endl;
   cout << "Sum=" << sum << endl;

   theApp.Run();
   return(0);

}

