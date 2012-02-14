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
#include <TF1.h>
#include <TROOT.h>
#include <TCanvas.h>
//#include <TMultiGraph.h>
//#include <TApplication.h>
#include <TGraph.h>
#include <TStyle.h>
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
#include <unistd.h>
using namespace std;

int chanMap[128];

void initChan ( ) {
	int idx;
	int chan;

	for ( idx = 0; idx < 128; idx++ ) {
		chan = (32*(idx%4)) + (8*(idx/4)) - (31*(idx/16));
		chanMap[chan] = idx;
	}
}


int convChan ( int chan ) {
	//return(chanMap[chan]);
	return(chan);
}


// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
	int c;
	int fit_sample = -1;
	TCanvas         *c1;
	TH2F            *histAll;
	TH1F            *histSng[640];
	double          histMin[640];
	double          histMax[640];
	uint hybridMin, hybridMax;
	TGraph          *graph;
	//TGraph          *sigma;
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
	uint            eventCount;
	double          avg;
	char            name[100];

	while ((c = getopt(argc,argv,"hs:")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-s: only read specified sample\n");
				return(0);
				break;
			case 's':
				fit_sample = atoi(optarg);
				break;
			case '?':
				printf("Invalid option or missing option argument; -h to list options\n");
				return(1);
			default:
				abort();
		}

	initChan();

	//gStyle->SetOptStat(kFALSE);
	gStyle->SetPalette(1,0);
	gROOT->SetStyle("Plain");

	// Start X11 view
	//   TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind != 1 ) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}


	// 2d histogram
	histAll = new TH2F("Value_Hist_All","Value_Hist_All",16384,-0.5,16383.5,640,-0.5,639.5);


	hybridMin = 16384;
	hybridMax = 0;
	for (channel=0; channel < 640; channel++) {
		sprintf(name,"%i",channel);
		histSng[channel] = new TH1F(name,name,16384,-0.5,16383.5);
		histMin[channel] = 16384;
		histMax[channel] = 0;
	}

	// Attempt to open data file
	if ( ! dataRead.open(argv[optind]) ) return(2);

	TString inname=argv[optind];

	inname.ReplaceAll(".bin","");
	if (inname.Contains('/')) {
		inname.Remove(0,inname.Last('/')+1);
	}
	ofstream outfile;
	if (fit_sample==-1)
	{
		cout << "Writing calibration to " << inname+".base" << endl;
		outfile.open(inname+".base");
	}
	else
	{
		cout << "Writing calibration to " << inname+".base_" <<fit_sample<< endl;
		sprintf(name,"%s.base_%d",inname.Data(),fit_sample);
		outfile.open(name);
	}

	// Process each event
	eventCount = 0;

	while ( dataRead.next(&event) ) {

		for (x=0; x < event.count(); x++) {

			// Get sample
			sample  = event.sample(x);
			channel = (sample->apv() * 128) + sample->channel();

			if ( channel >= (5 * 128) ) {
				cout << "Channel " << dec << channel << " out of range" << endl;
				cout << "Apv = " << dec << sample->apv() << endl;
				cout << "Chan = " << dec << sample->channel() << endl;
			}

			// Filter APVs
			if ( eventCount > 20 ) {

				avg = 0;
				for ( y=0; y < 6; y++ ) {
					if (fit_sample!=-1 && y!=fit_sample) continue;
					value = sample->value(y);

					//vhigh = (value << 1) & 0x2AAA;
					//vlow  = (value >> 1) & 0x1555;
					//value = vlow | vhigh;

					histAll->Fill(value,channel);
					histSng[channel]->Fill(value);

					if ( value < histMin[channel] ) histMin[channel] = value;
					if ( value > histMax[channel] ) histMax[channel] = value;
					if ( value < hybridMin ) hybridMin = value;
					if ( value > hybridMax ) hybridMax = value;
				}
			}
		}
		eventCount++;

	}

	for(channel = 0; channel < 640; channel++) {
		/*
		   if (histSng[channel]->Fit("gaus","Q0")==0) {
		   grMean[channel]  = histSng[channel]->GetFunction("gaus")->GetParameter(1);
		   grSigma[channel] = histSng[channel]->GetFunction("gaus")->GetParameter(2);
		   }
		   else {
		   cout << "Could not fit channel " << channel << endl;
		   grMean[channel]  = 0;
		   grSigma[channel] = 1;
		   }
		   */
		grMean[channel]  = histSng[channel]->GetMean();
		grSigma[channel]  = histSng[channel]->GetRMS();
		grChan[channel]  = channel;
		outfile <<channel<<"\t"<<grMean[channel]<<"\t"<<grSigma[channel]<<endl;     
	}

	c1 = new TCanvas("c1","c1",1200,900);
	c1->cd();
	histAll->GetXaxis()->SetRangeUser(hybridMin,hybridMax);
	histAll->Draw("colz");
	if (fit_sample==-1)
		sprintf(name,"%s_base.png",inname.Data());
	else 
		sprintf(name,"%s_base_%d.png",inname.Data(),fit_sample);
	c1->SaveAs(name);

	c1->Clear();
	graph = new TGraph(640,grChan,grMean);
	graph->SetTitle("base_pedestal");
	graph->GetXaxis()->SetRangeUser(0,640);
	graph->Draw("a*");
	if (fit_sample==-1)
		sprintf(name,"%s_base_pedestal.png",inname.Data());
	else 
		sprintf(name,"%s_base_pedestal_%d.png",inname.Data(),fit_sample);
	c1->SaveAs(name);

	c1->Clear();
	graph = new TGraph(640,grChan,grSigma);
	graph->SetTitle("base_noise");
	graph->GetXaxis()->SetRangeUser(0,640);
	graph->Draw("a*");
	if (fit_sample==-1)
		sprintf(name,"%s_base_noise.png",inname.Data());
	else 
		sprintf(name,"%s_base_noise_%d.png",inname.Data(),fit_sample);
	c1->SaveAs(name);

	// Start X-Windows
	//theApp.Run();

	// Close file
	dataRead.close();
	outfile.close();
	return(0);
	}

