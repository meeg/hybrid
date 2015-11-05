//-----------------------------------------------------------------------------
// File          : linkerror_ana.cpp
// Author        : Per Hansson Adrian  <phansson@slac.stanford.edu>
// Created       : 09/15/2015
// Project       : Kpix Software Package
//-----------------------------------------------------------------------------
// Description :
// File to look at link errors in data stream
//-----------------------------------------------------------------------------
// Copyright (c) 2009 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 09/15/2015: created
//-----------------------------------------------------------------------------

#include <iostream>
#include <sstream>
#include <cstdio>
#include "DataReadEvio.h"
#include "TiTriggerEvent.h"
#include "TriggerSample.h"
#include <TH2F.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TROOT.h>


using namespace std;

#define MAX_RCE 14
#define MAX_FEB 10
#define MAX_HYB 4


TiTriggerEvent triggerEvent;
TriggerSample* triggerSample;
int eventCount;
int sampleCount;
int numEvents = -1;
bool isHeadMultiSample;
int rce;
int hybrid;
int apv;
int channel;
int feb;
int debug;
int errorCountAll;
int errorCountHead;
ostringstream oss;
char name[200];
TH2F *baselineSamplesHist2D[MAX_RCE][MAX_FEB][MAX_HYB][6];
TH1F*sampleCountHist;
TCanvas *c1, *c2;
TString inname;




int main(int argc, char**argv ) {

  printf("JUST GO\n");
  
  if( argc < 2 ) {
    cout  << "need an evio file as argument." << endl;
    return 1;
  }

  if( argc > 2 ) {    
    cout << "set nr of events to " << argv[2] << endl;
    istringstream iss;
    iss.str(argv[2]);
    iss >> numEvents;
    cout << "set nr of events to " << numEvents << endl;
  }

  if( argc > 3 ) {       
    cout << "set debug level to " << argv[3] << endl;
    istringstream iss;
    iss.str(argv[3]);
    iss >> debug;
    cout << "set debug level to " << debug << endl;
  }

  if (inname=="")
    {
      inname=argv[optind];
      
      inname.ReplaceAll(".bin","");
      if (inname.Contains('/')) {
        inname.Remove(0,inname.Last('/')+1);
      }
    }



  DataReadEvio* dataRead = new DataReadEvio();
  dataRead->set_engrun(true);
  dataRead->set_bank_num(51);

  if( ! dataRead->open( argv[1] ) ) {
    cout << "couldn't open file " << argv[1] << endl;
    return 2;
  }
  
  cout << "Opened evio file \"" << argv[1] << "\"" << endl; 
  

  eventCount = 0;
  errorCountAll = 0;
  errorCountHead = 0;


  cout << "create histograms" << endl;
  for (int rce = 0;rce<MAX_RCE;rce++) {
    for (int fpga = 0;fpga<MAX_FEB;fpga++) {
      for (int hyb = 0;hyb<MAX_HYB;hyb++) {
        for (int sample = 0;sample<6;sample++) {
          oss.str("");        
          oss << "baselinesamples-rce-"<<rce<<"-fpga-"<<fpga<<"-hyb-"<<hyb<<"-sample-"<<sample;
          baselineSamplesHist2D[rce][fpga][hyb][sample] = new TH2F(oss.str().c_str(),oss.str().c_str(),640,0,640,10,0., 14000.0);
        }        
      }
    }
  }

  oss.str("");        
  oss << "sampleCountHist";
  sampleCountHist = new TH1F(oss.str().c_str(),oss.str().c_str(),50,0,50);


  cout << "read events" << endl;
  

  triggerSample = new TriggerSample();

  int tiEventNumber;
  int tiEventNumberPrev = -1;
  int tiEvents = 0;
  
  while( dataRead->next(&triggerEvent) == true ) {
    
    if( debug > 0 || (tiEvents % 10000 == 0) ) cout << "read event " << eventCount << " tiEvents " << tiEvents << endl;

    if( numEvents > 0 && eventCount > numEvents )
      break;
    
    tiEventNumber = triggerEvent.tiEventNumber();
    if(tiEventNumber > tiEventNumberPrev) {
      tiEventNumberPrev = tiEventNumber;
      tiEvents++;
    }
    
    sampleCount = triggerEvent.count();
    
    if( debug > 0) cout << "sampleCount: " << sampleCount << endl;
    
    sampleCountHist->Fill(sampleCount);

    for( int iSampleCount=0; iSampleCount!=sampleCount; ++iSampleCount) {
      
      if( debug > 0) cout << "iSampleCount: " << iSampleCount << endl;
      
      triggerEvent.sample(iSampleCount, triggerSample);

      rce = triggerSample->rceAddress();
      feb = triggerSample->febAddress();
      hybrid = triggerSample->hybrid();
      apv = triggerSample->apv();
      channel = triggerSample->channel();
      
      
      
      if( debug > 0) 
        cout << "rce: " << rce << " feb: " << feb << " hybrid: " << hybrid << " apv: " << apv << " channel: " << channel << endl;
            
      if( debug > 0)
        cout << "HEAD " << (triggerSample->head() ? 1 : 0) << " TAIL " << (triggerSample->tail() ? 1 : 0) << " ERR " << (triggerSample->error() ? 1 : 0);

      if( debug > 0) cout << " ADC samples: ";
      for(int y = 0; y < 6; ++y) {
        uint val = triggerSample->value( y );
        baselineSamplesHist2D[rce][feb][hybrid][y]->Fill(channel,val);
        
        if( debug > 0) cout << " " << val;
      } 
      if( debug > 0) cout << endl;
      

      
    } // iSampleCount
    
    
    eventCount++;
  } // while data read OK
  
  
  printf("Clean data read\n");
  dataRead->close();
  delete dataRead;
  delete triggerSample;
  
  cout << "File " << argv[1] << " tiEvents " << tiEvents << " errorCountAll " << errorCountAll << " errorCountHead: " << errorCountHead << endl;
  




  gROOT->SetStyle("Plain");
  gStyle->SetOptStat("emrou");
  gStyle->SetPalette(1,0);
  gStyle->SetStatW(0.2);                
  gStyle->SetStatH(0.1);                
  gStyle->SetTitleOffset(1.4,"y");
  gStyle->SetPadLeftMargin(0.15);
  gStyle->SetMarkerStyle(6);
  c1 = new TCanvas("c1","c1",1200,900);
  c2 = new TCanvas("c2","c2",1200,900);
  
  sprintf(name,"%s_baselinesamples.ps[",inname.Data());
  c2->Print(name);

  c2->Clear();
  c2->cd();
  sampleCountHist->Draw();
  sprintf(name,"%s_baselinesamples.ps",inname.Data());
  c2->Print(name);
  

  for (int rce = 0;rce<MAX_RCE;rce++) {
    for (int fpga = 0;fpga<MAX_FEB;fpga++) {
      for (int hyb = 0;hyb<MAX_HYB;hyb++) {        
        for(int sample=0; sample<6; ++sample) {
          c2->Clear();            
          c2->cd();
          sprintf(name,"%s_baselinesamples_R%d_F%d_H%d_S%d.png",inname.Data(),rce,fpga,hyb,sample);
          printf("Drawing %s\n",name);
          baselineSamplesHist2D[rce][fpga][hyb][sample]->Draw("colz");
          //c2->SaveAs(name);
          sprintf(name,"%s_baselinesamples.ps",inname.Data());
          c2->Print(name);
        }
      }
    }
  }
  
  sprintf(name,"%s_baselinesamples.ps]",inname.Data());
  c1->Print(name);

  return 0;
  

} // main
