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
#include <algorithm>
#include <cmath>
#include <TFile.h>
#include <TH1F.h>
#include <meeg_utils.hh>
#include <TH2F.h>
#include <TF1.h>
#include <TROOT.h>
#include <TCanvas.h>
#include <TMultiGraph.h>
//#include <TApplication.h>
#include <TGraph.h>
#include <TStyle.h>
#include <TLegend.h>
#include <stdarg.h>
#include <DevboardEvent.h>
#include <DevboardSample.h>
#include <TiTriggerEvent.h>
#include <TriggerSample.h>
#include <Data.h>
#include <DataRead.h>
#include <DataReadEvio.h>
#include <unistd.h>
using namespace std;

#define MAX_RCE 14
#define MAX_FEB 10
#define MAX_HYB 4


int main ( int argc, char **argv ) {

  printf("hej1\n");

  bool flip_channels = true;
  bool mux_channels = false;
  bool read_temp = true;
  int hybrid_type = 0;
  bool evio_format = false;
  bool triggerevent_format = false;
  int use_fpga = -1;
  int use_hybrid = -1;
  int num_events = -1;
  double threshold_sigma = 2.0;
  int c;
  bool doPulseShape = false;
  int pulseShapeRce = -1;
  int pulseShapeCh = -1;
  int pulseShapeFeb = -1;
  int pulseShapeHyb = -1;
  const int nTimeBins = 10;
  TCanvas         *c1, *c2;
  int chanMap[128];
  for (int idx = 0; idx < 128; idx++ ) {
    chanMap[(32*(idx%4)) + (8*(idx/4)) - (31*(idx/16))] = idx;
  }
  ostringstream oss;

  TH2F* pulseShapeHist[MAX_RCE][MAX_FEB][MAX_HYB][640];
  int hybridCount[MAX_RCE][MAX_FEB][MAX_HYB];
  int *channelCount[MAX_RCE][MAX_FEB][MAX_HYB];
  int *channelAllCount[MAX_RCE][MAX_FEB][MAX_HYB];
  TH2F *baselineHist2D[MAX_RCE][MAX_FEB][MAX_HYB];
  TH2F *baselineSamplesHist2D[MAX_RCE][MAX_FEB][MAX_HYB][6];
  TH2F *baselineSamples0TimeBinHist2D[MAX_RCE][MAX_FEB][MAX_HYB][nTimeBins];
  TH1F* timeStampDiffHist;
  TH1F* timeStampDiffBinHist;

  double          grChan[640];
  for (int i=0;i<640;i++){
    grChan[i] = i;
  }
  DataRead        *dataRead;
  int svt_bank_num = 3;
  DevboardEvent    event;
  TiTriggerEvent    triggerevent;
  TriggerSample   *triggersample = new TriggerSample();
  int		samples[6];
  int            eventCount;
  int            tiEventCount;
  int runCount = -1;
  TString inname;
  TString outdir;
  char            name[200];

  printf("hej11\n");

    
  while ((c = getopt(argc,argv,"ho:nmct:H:F:e:Es:b:VA:B:C:D:")) !=-1)
    switch (c)
      {
      case 'h':
        printf("-h: print this help\n");
        printf("-o: use specified output filename\n");
        printf("-n: DAQ (Ryan's) channel numbering\n");
        printf("-m: number channels in raw mux order\n");
        printf("-c: don't compute correlations\n");
        printf("-t: hybrid type (1 for old test run hybrid, 2 for new 2014 hybrid)\n");
        printf("-F: use only specified FPGA\n");
        printf("-H: use only specified hybrid\n");
        printf("-e: stop after specified number of events\n");
        printf("-E: use EVIO file format\n");
        printf("-s: number of sigmas for threshold file (default 2)\n");
        printf("-b: EVIO bank number for SVT (default 3)\n");
        printf("-V: use TriggerEvent event format\n");
        printf("-A: Rce to use for pulse shape plots\n");
        printf("-B: Feb to use for pulse shape plots\n");
        printf("-C: Hybrid to use for pulse shape plots\n");
        printf("-D: sensor channel to use for pulse shape plots\n");        
        return(0);
        break;
      case 'o':
        inname = optarg;
        outdir = optarg;
        if (outdir.Contains('/')) {
          outdir.Remove(outdir.Last('/')+1);
        }
        else outdir="";
        break;
      case 'n':
        flip_channels = false;
        break;
      case 'm':
        mux_channels = true;
        break;
      case 't':
        hybrid_type = atoi(optarg);
        break;
      case 'F':
        use_fpga = atoi(optarg);
        break;
      case 'H':
        use_hybrid = atoi(optarg);
        break;
      case 'e':
        num_events = atoi(optarg);
        break;
      case 'E':
        evio_format = true;
        break;
      case 's':
        threshold_sigma = atof(optarg);
        break;
      case 'b':
        svt_bank_num = atoi(optarg);
        break;
      case 'V':
        triggerevent_format = true;
        break;
      case 'A':
        pulseShapeRce = atoi(optarg);
        break;
      case 'B':
        pulseShapeFeb = atoi(optarg);
        break;
      case 'C':
        pulseShapeHyb = atoi(optarg);
        break;
      case 'D':
        pulseShapeCh = atoi(optarg);
        break;
      case '?':
        printf("Invalid option or missing option argument; -h to list options\n");
        return(1);
      default:
        abort();
      }
    
  printf("hej111\n");
    
  if (hybrid_type==0) {
    printf("WARNING: no hybrid type set; use -t to specify old or new hybrid\n");
    printf("Configured for old (test run) hybrid\n");
    hybrid_type = 1;
  }

  if (evio_format) {
    DataReadEvio *tmpDataRead = new DataReadEvio();
    if (!triggerevent_format)
      tmpDataRead->set_bank_num(svt_bank_num);
    else
      {
        tmpDataRead->set_engrun(true);
        tmpDataRead->set_bank_num(51);
      }
    dataRead = tmpDataRead;
     cout << "use evio_format reader\n";
  } else 
    dataRead = new DataRead();

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

  // Start X11 view
  //TApplication theApp("App",NULL,NULL);

  // Root file is the first and only arg
  if ( argc-optind != 1 ) {
    cout << "Usage: pelle_baseline data_file\n";
    return(1);
  }
  
  if (inname=="")
    {
      inname=argv[optind];
      
      inname.ReplaceAll(".bin","");
      if (inname.Contains('/')) {
        inname.Remove(0,inname.Last('/')+1);
      }
    }
  
  cout << "Reading data file " <<argv[optind] << endl;
  // Attempt to open data file
  if ( ! dataRead->open(argv[optind]) ) return(2);
  
  // TString confname=argv[optind];
  // confname.ReplaceAll(".bin","");
  // confname.Append(".conf");
  // if (confname.Contains('/')) {
  //   confname.Remove(0,confname.Last('/')+1);
  // }
  

  // ofstream outconfig;
  // cout << "Writing configuration to " <<outdir<<confname << endl;
  // outconfig.open(outdir+confname);

  bool readOK;

  cout << "Get first event " << endl;

  if (triggerevent_format) {
    dataRead->next(&triggerevent);
  } else {
    dataRead->next(&event);
  }
  /*
  outconfig << dataRead->getConfigXml();
  outconfig << endl;
  outconfig << dataRead->getStatusXml();
  outconfig.close();
  
  runCount = atoi(dataRead->getConfig("RunCount").c_str());
  */

  // Process each event
  eventCount = 0;
  tiEventCount = 0;



  printf("Init some histos\n");

  oss.str("");
  oss << "timeStampDiff";
  timeStampDiffHist = new TH1F(oss.str().c_str(),oss.str().c_str(),100,0,100.);
  oss << "timeStampDiffBin";
  timeStampDiffBinHist = new TH1F(oss.str().c_str(),oss.str().c_str(),nTimeBins,0,nTimeBins);


  for (int rce = 0;rce<MAX_RCE;rce++) {
    for (int fpga = 0;fpga<MAX_FEB;fpga++) {
      for (int hyb = 0;hyb<MAX_HYB;hyb++) {

        hybridCount[rce][fpga][hyb] = 0;

        oss.str("");
        oss << "baseline-rce-"<<rce<<"-fpga-"<<fpga<<"-hyb-"<<hyb;
        baselineHist2D[rce][fpga][hyb] = new TH2F(oss.str().c_str(),oss.str().c_str(),640,0,640,10,0., 14000.0);
        
        for (int sample = 0;sample<6;sample++) {
          oss.str("");        
          oss << "baselinesamples-rce-"<<rce<<"-fpga-"<<fpga<<"-hyb-"<<hyb<<"-sample-"<<sample;
          baselineSamplesHist2D[rce][fpga][hyb][sample] = new TH2F(oss.str().c_str(),oss.str().c_str(),640,0,640,10,0., 14000.0);
        }

        for (int tb = 0;tb<nTimeBins;tb++) {
          oss.str("");        
          oss << "baselinesample0timebin-rce-"<<rce<<"-fpga-"<<fpga<<"-hyb-"<<hyb<<"-sample0-timebin-"<<tb;
          baselineSamples0TimeBinHist2D[rce][fpga][hyb][tb] = new TH2F(oss.str().c_str(),oss.str().c_str(),640,0,640,10,0., 14000.0);
        }
        
        for (int ch =0;ch<640;ch++) {          
          pulseShapeHist[rce][fpga][hyb][ch] = NULL;
        }        
      }
    }
  }


  unsigned long timeStamp;
  unsigned long tiEventNumber;
  unsigned long timeStampPrev = 0;
  unsigned long tiEventNumberPrev = 0;
  unsigned long timeStampDiff = 0;
  double timeStampDiffD = 0;
  uint tiTimeBin = 0;


  do {
    int rce = 0;
    int fpga = 0;
    int samplecount;

    if (triggerevent_format) {
      samplecount = triggerevent.count();
      //printf("datacode %d, sequence %d, samplecount %d\n",triggerevent.dataEventCode(),triggerevent.sequence(),samplecount);
    } else {
      fpga = event.fpgaAddress();
      samplecount = event.count();
      if (read_temp && !event.isTiFrame()) for (uint i=0;i<4;i++) {
          printf("Event %d, temperature #%d: %f\n",eventCount,i,event.temperature(i,hybrid_type==1));
          read_temp = false;
        }
    }

    if (!triggerevent_format && fpga==7) 
      {
        //printf("not a data event\n");
        continue;
      }
    //cout<<"  fpga #"<<event.fpgaAddress()<<"; number of samples = "<<event.count()<<endl;
    if (eventCount%1000==0) printf("Event %d\n",eventCount);    
    if (num_events!=-1 && eventCount >= num_events) break;

    for (int x=0; x < samplecount; x++) {
      int hyb;
      int apv;
      int apvch;
      int channel;
      

      bool goodSample = true;

      // Get sample
      if (triggerevent_format) {
        triggerevent.sample(x,triggersample);
        rce = triggersample->rceAddress();
        fpga = triggersample->febAddress();
        hyb = triggersample->hybrid();
        apv = triggersample->apv();
        apvch = triggersample->channel();
        goodSample = (!triggersample->head() && !triggersample->tail());
        for ( int y=0; y < 6; y++ ) {
          //printf("%x\n",sample->value(y));
          samples[y] = triggersample->value(y);
        }
      } else {
        DevboardSample *sample  = event.sample(x);
        hyb = sample->hybrid();
        apv = sample->apv();
        apvch = sample->channel();
        for ( int y=0; y < 6; y++ ) {
          //printf("%x\n",sample->value(y));
          samples[y] = sample->value(y) & 0x3FFF;
          if (samples[y]==0) goodSample = false;
        }
      }
      if (use_fpga!=-1 && fpga!=use_fpga) continue;
      if (use_hybrid!=-1 && hyb!=use_hybrid) continue;
      if (!goodSample) continue;
      //printf("fpga %d, hybrid %d, apv %d, chan %d\n",fpga,hyb,apv,apvch);
      //printf("event %d\tx=%d\tR%d F%d H%d A%d channel %d, samples:\t%d\t%d\t%d\t%d\t%d\t%d\n",eventCount,x,rce,fpga,hyb,apv,apvch,samples[0],samples[1],samples[2],samples[3],samples[4],samples[5]);
      
      if (mux_channels) channel = chanMap[apvch];
      else channel = apvch;
      
      if (flip_channels)
        channel += (4-apv)*128;
      else
        channel += apv*128;
      
      //if (eventCount==0) printf("channel %d\n",channel);
      
      if ( channel >= (5 * 128) ) {
        cout << "Channel " << dec << channel << " out of range" << endl;
        cout << "Apv = " << dec << apv << endl;
        cout << "Chan = " << dec << apvch << endl;
        exit(1);
      }
      
      
      // Filter APVs
      if ( eventCount >= 20 ) {
        if (hybridCount[rce][fpga][hyb]==0) {
          printf("found new hybrid: rce = %d, feb = %d, hyb = %d\n",rce,fpga,hyb);
          channelCount[rce][fpga][hyb] = new int[640];
          channelAllCount[rce][fpga][hyb] = new int[640];
          for (int i=0;i<640;i++) {
            channelCount[rce][fpga][hyb][i] = 0;
            channelAllCount[rce][fpga][hyb][i] = 0;
          }
        }
        hybridCount[rce][fpga][hyb]++;
        channelCount[rce][fpga][hyb][channel]++;


        // calculate the time to the previous trigger
        if(triggerevent.tiEventNumber()>tiEventNumber) {
          tiEventCount++;
          // it's a new event, save the old ones
          tiEventNumberPrev = tiEventNumber;
          timeStampPrev = timeStamp;
          // update the current ones
          tiEventNumber = triggerevent.tiEventNumber();
          timeStamp = triggerevent.timeStamp();
          // difference in time
          timeStampDiff = timeStamp-timeStampPrev;
          timeStampDiffD = 4.0e-3*((double)timeStamp-timeStampPrev);
          
          timeStampDiffHist->Fill(timeStampDiffD);
          tiTimeBin = (uint)(timeStampDiffD/10.0);
          if(tiTimeBin>=nTimeBins) tiTimeBin=9;
          timeStampDiffBinHist->Fill(tiTimeBin);

        }
        //cout << timeStampDiff <<  " 4ns clocks ( "<< timeStampDiffD <<" us) -> timebin (25us bins) " <<  tiTimeBin << endl;
        

        if(tiEventCount%100==0) {
          printf("processing hybrid: rce = %d, feb = %d, hyb = %d\n",rce,fpga,hyb);        
          if(triggerevent_format) {
            printf("TI event number %ld (prev %ld)\n",tiEventNumber, tiEventNumberPrev);
            printf("time stamp %ld (prev %ld diff %ld tiTimeBin %d)\n",timeStamp, timeStampPrev, timeStampDiff, tiTimeBin);
          }
        }

        
        if ( doPulseShape && pulseShapeHist[rce][fpga][hyb][channel] == NULL) {
          oss.str("");
          oss << "pulseShapeHist_" << rce << "_" << fpga << "_" << hyb << "_" << channel;
          cout << "Create histogram " << oss.str() << endl;
          pulseShapeHist[rce][fpga][hyb][channel] = new TH2F(oss.str().c_str(),oss.str().c_str(),6,0,6,50,0,10000);
        }

        for ( int y=0; y < 6; y++ ) {
          //printf("%x\n",sample->value(y));
          int value = samples[y];
          if (value<1000) {
            printf("out of range: event %d, rce = %d, feb = %d, hyb = %d, channel = %d, sample[%d] = %d\n",eventCount,rce,fpga,hyb,channel,y,samples[y]);
          }
                    
          baselineHist2D[rce][fpga][hyb]->Fill(channel,value);
          baselineSamplesHist2D[rce][fpga][hyb][y]->Fill(channel,value);
          baselineSamples0TimeBinHist2D[rce][fpga][hyb][tiTimeBin]->Fill(channel,value);
          
          channelAllCount[rce][fpga][hyb][channel]++;
          
          if ( doPulseShape) 
            pulseShapeHist[rce][fpga][hyb][channel]->Fill(y,value);
          
        } //y=samples
      } // eventCount>20
    } // x=samplecount
    eventCount++;

    
    if (triggerevent_format) {
      readOK = dataRead->next(&triggerevent);
    } else {
      readOK = dataRead->next(&event);
    }
    
  } while (readOK);

  dataRead->close();
  
  if (!evio_format && eventCount != runCount)
    {
      printf("ERROR: events read = %d, runCount = %d\n",eventCount, runCount);
    }
  

  printf("Process summary:\nEventCount = %d\nTiEventCount=%d\n",eventCount,tiEventCount);


  c1->Clear();
  sprintf(name,"%s_timestampdiff.png",inname.Data());
  printf("Drawing %s\n",name);
  timeStampDiffHist->Draw();
  c1->SaveAs(name);

  c1->Clear();
  sprintf(name,"%s_timestampdiffbin.png",inname.Data());
  printf("Drawing %s\n",name);
  timeStampDiffBinHist->Draw();
  c1->SaveAs(name);

  
  TH2F* pulseShapeHistPerHybrid;
  TH1F* occupancy[MAX_RCE][MAX_FEB][MAX_HYB];
  TH1F* occupancytimebin[MAX_RCE][MAX_FEB][MAX_HYB][nTimeBins];
  for (int rce = 0;rce<MAX_RCE;rce++) {
    for (int fpga = 0;fpga<MAX_FEB;fpga++) {
      for (int hyb = 0;hyb<MAX_HYB;hyb++) {
        if (hybridCount[rce][fpga][hyb]) {
          
          pulseShapeHistPerHybrid = NULL;
          
          if(doPulseShape) {
            for (int ch=0;ch<640;ch++) {
            
              if(  pulseShapeHist[rce][fpga][hyb][ch] != NULL) {             
                if(pulseShapeHistPerHybrid==NULL) {
                  sprintf(name,"%s_pulseshape_R%d_F%d_H%d.png",inname.Data(),rce,fpga,hyb);         
                  pulseShapeHistPerHybrid = (TH2F*)  pulseShapeHist[rce][fpga][hyb][ch]->Clone(name);
                } 
                else {
                  pulseShapeHistPerHybrid->Add(pulseShapeHist[rce][fpga][hyb][ch]);
                }
              
                // do individual channels if wanted
                if(fpga == pulseShapeFeb && pulseShapeHyb == hyb && pulseShapeRce == rce && pulseShapeCh == ch) {
                  sprintf(name,"%s_pulseshape_R%d_F%d_H%d_CH%d.png",inname.Data(),rce,fpga,hyb,ch);
                  printf("Drawing %s\n",name);
                  c1->Clear();
                  printf("Drawing %s\n",name);
                  pulseShapeHist[rce][fpga][hyb][ch]->Draw("colz");
                  printf("Drawing %s\n",name);
                  c1->SaveAs(name);
                  printf("Saved %s\n",name);
                }             
              }
            
            } //ch
          
          

            if( pulseShapeHistPerHybrid != NULL ) {
              sprintf(name,"%s_pulseshape_R%d_F%d_H%d.png",inname.Data(),rce,fpga,hyb);
              printf("Drawing %s\n",name);
              c1->Clear();
              printf("Drawing %s\n",name);
              pulseShapeHistPerHybrid->Draw("colz");
              printf("Drawing %s\n",name);
              c1->SaveAs(name);
              printf("Saved %s\n",name);
              delete pulseShapeHistPerHybrid;
            }
          }


          c1->Clear();
          
          sprintf(name,"%s_baseline_R%d_F%d_H%d.png",inname.Data(),rce,fpga,hyb);
          printf("Drawing %s\n",name);
          baselineHist2D[rce][fpga][hyb]->Draw("colz");
          c1->SaveAs(name);

          c1->Clear();          
          sprintf(name,"%s_occupancy_R%d_F%d_H%d.png",inname.Data(),rce,fpga,hyb);
          printf("Drawing %s\n",name);
          occupancy[rce][fpga][hyb] = (TH1F*) baselineHist2D[rce][fpga][hyb]->ProjectionX();
          if (tiEventCount>0) occupancy[rce][fpga][hyb]->Scale(1.0/tiEventCount);
          else  occupancy[rce][fpga][hyb]->Scale(0.0); // set to 0          
          occupancy[rce][fpga][hyb]->Draw();
          //c1->SetLogy();
          c1->SaveAs(name);
          // for(int bin=1; bin<prjY->GetNbinsX()+1; ++bin) {
          //   double n = prjY->GetBinContent(bin);
          //   double occ = 0.0;
          //   if (eventCount>0) {
          //     occ = n/eventCount;
          //   }
          //   occupancy[rce][fpga][hyb][sample]->ProjectionY();
          // }
          // delete prjY;
          

          for(int sample=0; sample<6; ++sample) {
            c1->Clear();            
            sprintf(name,"%s_baselinesamples_R%d_F%d_H%d_S%d.png",inname.Data(),rce,fpga,hyb,sample);
            printf("Drawing %s\n",name);
            baselineSamplesHist2D[rce][fpga][hyb][sample]->Draw("colz");
            c1->SaveAs(name);
          }

          
          c2->Clear();
          TLegend* leg = new TLegend(0.35,0.5,0.73,0.85);
          leg->SetFillColor(0);
          Color_t color;
          for(int timebin=0; timebin<nTimeBins; ++timebin) {
            c1->cd();
            c1->Clear();            
            sprintf(name,"%s_baselinesamples0timebins_R%d_F%d_H%d_T%d.png",inname.Data(),rce,fpga,hyb,timebin);
            printf("Drawing %s\n",name);
            baselineSamples0TimeBinHist2D[rce][fpga][hyb][timebin]->Draw("colz");
            c1->SaveAs(name);

            c1->Clear();          
            sprintf(name,"%s_occupancy_timebins_R%d_F%d_H%d_T%d.png",inname.Data(),rce,fpga,hyb,timebin);
            printf("Drawing %s\n",name);
            occupancytimebin[rce][fpga][hyb][timebin] = (TH1F*) baselineSamples0TimeBinHist2D[rce][fpga][hyb][timebin]->ProjectionX();
            // find the number of ti events in this time bin to normalize correctly
            uint tiEventCountTimeBin = timeStampDiffBinHist->GetBinContent(timebin+1);
            if (tiEventCountTimeBin>0) occupancytimebin[rce][fpga][hyb][timebin]->Scale(1.0/tiEventCountTimeBin);
            else  occupancytimebin[rce][fpga][hyb][timebin]->Scale(0.0); // set to 0          
            occupancytimebin[rce][fpga][hyb][timebin]->Draw();
            myText(0.6,0.6,TString::Format("%d events in timebin",tiEventCountTimeBin).Data(),0.05,1);
            //c1->SetLogy();
            c1->SaveAs(name);

            c2->cd();
            if(timebin<9) color = (Color_t)timebin+1;
            else color = (Color_t)timebin+2;
            occupancytimebin[rce][fpga][hyb][timebin]->SetLineColor(color);
            if(timebin==0)
              occupancytimebin[rce][fpga][hyb][timebin]->Draw();
            else
              occupancytimebin[rce][fpga][hyb][timebin]->Draw("same");
            leg->AddEntry( occupancytimebin[rce][fpga][hyb][timebin], TString::Format("#DeltaT_{bin} %d#mus",timebin*10).Data(),"L");
            
          }

          sprintf(name,"%s_occupancy_timebinsAll_R%d_F%d_H%d.png",inname.Data(),rce,fpga,hyb);
          printf("Drawing %s\n",name);
          c2->cd();
          leg->Draw();
          c2->SaveAs(name);
          delete leg;

          
          



          for(int sample=0; sample<6; ++sample) {
            c1->Clear();            
            sprintf(name,"%s_baselinesamples_R%d_F%d_H%d_S%d.png",inname.Data(),rce,fpga,hyb,sample);
            printf("Drawing %s\n",name);
            c1->SaveAs(name);
          }

          
        }
      }
    }
  }
  // Start X-Windows
  //theApp.Run();
  
  // Close file
  delete dataRead;
  return(0);
}

