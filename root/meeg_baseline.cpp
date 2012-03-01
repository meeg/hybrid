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
#include <TMultiGraph.h>
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

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
	int c;
	TCanvas         *c1;
	TH2F            *histAll[7];
	TH1F            *histSng[640][7];
	double          histMin[640];
	double          histMax[640];
	uint hybridMin, hybridMax;
	//TGraph          *sigma;
	int ni = 0;
	double          grChan[640];
	double          grMean[7][640];
	double          grSigma[7][640];
	DataRead        dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	uint            value;
	uint            channel;
	uint            eventCount;
	double          avg;
	TString inname;
	char            name[100];
	char title[200];

	while ((c = getopt(argc,argv,"ho:")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-o: use specified output filename\n");
				return(0);
				break;
			case 'o':
				inname = optarg;
				break;
			case '?':
				printf("Invalid option or missing option argument; -h to list options\n");
				return(1);
			default:
				abort();
		}

	gROOT->SetStyle("Plain");
	gStyle->SetOptStat("emrou");
	gStyle->SetPalette(1,0);
	gStyle->SetStatW(0.2);                
	gStyle->SetStatH(0.1);                
	gStyle->SetTitleOffset(1.4,"y");
	gStyle->SetPadLeftMargin(0.15);
	c1 = new TCanvas("c1","c1",1200,900);

	// Start X11 view
	//   TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind != 1 ) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}


	// 2d histogram
	for (int i=0;i<6;i++)
	{
		sprintf(name,"Value_Hist_s%d",i);
		sprintf(title,"Baseline values, sample %d;ADC counts;Channel",i);
		histAll[i] = new TH2F(name,title,16384,-0.5,16383.5,640,-0.5,639.5);
	}
	sprintf(title,"Baseline values, all samples;ADC counts;Channel");
	histAll[6] = new TH2F("Value_Hist_All",title,16384,-0.5,16383.5,640,-0.5,639.5);


	hybridMin = 16384;
	hybridMax = 0;
	for (channel=0; channel < 640; channel++) {
		for (int i=0;i<6;i++)
		{
			sprintf(name,"ch_%i_sample_%d",channel,i);
			histSng[channel][i] = new TH1F(name,name,16384,-0.5,16383.5);
		}
		sprintf(name,"ch_%i_all_samples",channel);
		histSng[channel][6] = new TH1F(name,name,16384,-0.5,16383.5);
		histMin[channel] = 16384;
		histMax[channel] = 0;
	}

	if (inname=="")
	{
		inname=argv[optind];

		inname.ReplaceAll(".bin","");
		if (inname.Contains('/')) {
			inname.Remove(0,inname.Last('/')+1);
		}
	}
	ofstream outfile;
	cout << "Writing calibration to " << inname+".base" << endl;
	outfile.open(inname+".base");

	// Attempt to open data file
	if ( ! dataRead.open(argv[optind]) ) return(2);

	dataRead.next(&event);
	dataRead.dumpConfig();
	//dataRead.dumpStatus();
	//for (uint i=0;i<4;i++)
	//	printf("Temperature #%d: %f\n",i,event.temperature(i));

	// Process each event
	eventCount = 0;

	do {
		if (eventCount%1000==0) printf("Event %d\n",eventCount);
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
					value = sample->value(y);

					//vhigh = (value << 1) & 0x2AAA;
					//vlow  = (value >> 1) & 0x1555;
					//value = vlow | vhigh;

					histAll[y]->Fill(value,channel);
					histSng[channel][y]->Fill(value);
					histAll[6]->Fill(value,channel);
					histSng[channel][6]->Fill(value);

					if ( value < histMin[channel] ) histMin[channel] = value;
					if ( value > histMax[channel] ) histMax[channel] = value;
					if ( value < hybridMin ) hybridMin = value;
					if ( value > hybridMax ) hybridMax = value;
				}
			}
		}
		eventCount++;

	} while ( dataRead.next(&event));
	dataRead.close();

	for(channel = 0; channel < 640; channel++) {
		if (histSng[channel][6]->GetEntries()!=0)
		{
			grChan[ni]  = channel;
			outfile <<channel<<"\t";
			for (int i=0;i<7;i++)
			{
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
				grMean[i][ni]  = histSng[channel][i]->GetMean();
				grSigma[i][ni]  = histSng[channel][i]->GetRMS();
				outfile<<grMean[i][ni]<<"\t"<<grSigma[i][ni]<<"\t";
			}
			outfile<<endl;
			ni++;
		}
	}

	TGraph          *graph[7];
	TMultiGraph *mg;

	histAll[6]->GetXaxis()->SetRangeUser(hybridMin,hybridMax);
	histAll[6]->Draw("colz");
	sprintf(name,"%s_base.png",inname.Data());
	c1->SaveAs(name);

	for (int i=0;i<6;i++)
	{
		histAll[i]->GetXaxis()->SetRangeUser(hybridMin,hybridMax);
		histAll[i]->Draw("colz");
		sprintf(name,"%s_base_%d.png",inname.Data(),i);
		c1->SaveAs(name);
	}

	c1->Clear();
	mg = new TMultiGraph();
	for (int i=0;i<7;i++)
	{
		graph[i] = new TGraph(ni,grChan,grMean[i]);
		graph[i]->SetMarkerColor(i+2);
		mg->Add(graph[i]);
	}
	graph[6]->SetMarkerColor(1);
	mg->SetTitle("Pedestal;Channel;ADC counts");
	//mg->GetXaxis()->SetRangeUser(0,640);
	mg->Draw("a*");
	sprintf(name,"%s_base_pedestal.png",inname.Data());
	c1->SaveAs(name);
	for (int i=0;i<7;i++)
		delete graph[i];
	delete mg;

	c1->Clear();
	mg = new TMultiGraph();
	for (int i=0;i<7;i++)
	{
		graph[i] = new TGraph(ni,grChan,grSigma[i]);
		graph[i]->SetMarkerColor(i+2);
		mg->Add(graph[i]);
	}
	graph[6]->SetMarkerColor(1);
	mg->SetTitle("Noise;Channel;ADC counts");
	//mg->GetXaxis()->SetRangeUser(0,640);
	mg->Draw("a*");
	sprintf(name,"%s_base_noise.png",inname.Data());
	c1->SaveAs(name);
	for (int i=0;i<7;i++)
		delete graph[i];
	delete mg;

	// Start X-Windows
	//theApp.Run();

	// Close file
	outfile.close();
	return(0);
}

