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
	TCanvas         *c1;
	TH2F            *histAll;
	TH1F            *histSng[640];
	double          histMin[640];
	double          histMax[640];
	int hybridMin, hybridMax;
	//TGraph          *mean;
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

	initChan();

	//gStyle->SetOptStat(kFALSE);
	gStyle->SetPalette(1,0);

	// Start X11 view
	//   TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc != 2 && argc != 3 ) {
		cout << "Usage: meeg_baseline data_file [sample #]\n";
		return(1);
	}


	int fit_sample = -1;
	if (argc==3)
		fit_sample=atoi(argv[2]);


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
	if ( ! dataRead.open(argv[1]) ) return(2);

	TString inname=argv[1];

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

	/*
	   c2 = new TCanvas("c2","c2");
	   c2->Divide(12,11,0.01);

	   int base = 3 * 128;

	   for(channel = base; channel < (base+128); channel++) {
	   c2->cd((channel-base)+1);
	   histSng[channel]->GetXaxis()->SetRangeUser(histMin[channel],histMax[channel]);
	   histSng[channel]->Draw();
	   }

	   c4 = new TCanvas("c4","c4");
	   c4->cd();
	   mean = new TGraph(640,grChan,grMean);
	   mean->Draw("a*");

	   c5 = new TCanvas("c5","c5");
	   c5->cd();
	   sigma = new TGraph(640,grChan,grSigma);
	   sigma->Draw("a*");
	   */

	// Start X-Windows
	//theApp.Run();

	// Close file
	dataRead.close();
	outfile.close();
	return(0);
}

