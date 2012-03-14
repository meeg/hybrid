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
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
#include <unistd.h>
using namespace std;

//#define corr1 384
//#define corr2 383

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
	bool flip_channels = false;
	bool mux_channels = false;
	bool skip_corr = false;
	int c;
	TCanvas         *c1;
	TH2F            *histAll[7];
	short *allSamples[640][7];
	for (int i=0;i<640;i++) for (int j=0;j<7;j++)
	{
		allSamples[i][j] = new short[16384];
		for (int k=0;k<16384;k++) allSamples[i][j][k]=0;
	}
	int chanMap[128];
	for (int idx = 0; idx < 128; idx++ ) {
		chanMap[(32*(idx%4)) + (8*(idx/4)) - (31*(idx/16))] = idx;
	}

	bool channelActive[640];
	double channelAvg[640];
	int channelCount[640];
	double channelMean[640];
	double channelVariance[640];
	double channelCovar[640][640];
	for (int i=0;i<640;i++) {
		channelCount[i] = 0;
		channelMean[i] = 0.0;
		channelVariance[i] = 0.0;
		for (int j=0;j<640;j++) {
			channelCovar[i][j] = 0.0;
		}
	}
//	TH2F *corrHist = new TH2F("corrHist","Channel correlation",16384,-0.5,16383.5,16384,-0.5,16383.5);

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
	TString inname;
	TString outdir;
	char            name[100];
	char title[200];

	while ((c = getopt(argc,argv,"ho:nmc")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-o: use specified output filename\n");
				printf("-n: flip channel numbering\n");
				printf("-m: number channels in raw mux order\n");
				printf("-c: don't compute correlations\n");
				return(0);
				break;
			case 'o':
				inname = optarg;
				outdir = optarg;
				if (outdir.Contains('/')) {
					outdir.Remove(outdir.Last('/'),outdir.Length());
				}
				else outdir="";
				break;
			case 'n':
				flip_channels = true;
				break;
			case 'c':
				skip_corr = true;
				break;
			case 'm':
				mux_channels = true;
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

	cout << "Reading data file " <<argv[optind] << endl;
	// Attempt to open data file
	if ( ! dataRead.open(argv[optind]) ) return(2);

	TString confname=argv[optind];
	confname.ReplaceAll(".bin",".conf");
	if (confname.Contains('/')) {
		confname.Remove(0,confname.Last('/')+1);
	}

	ofstream outconfig;
	cout << "Writing configuration to " <<outdir<<confname << endl;
	outconfig.open(outdir+confname);

	dataRead.next(&event);
	dataRead.dumpConfig(outconfig);
	outconfig.close();
	//dataRead.dumpStatus();
	//for (uint i=0;i<4;i++)
	//	printf("Temperature #%d: %f\n",i,event.temperature(i));



	// Process each event
	eventCount = 0;

	do {
		if (eventCount%1000==0) printf("Event %d\n",eventCount);
		for (int i=0;i<640;i++)
		{
			channelActive[i] = false;
		}
		for (x=0; x < event.count(); x++) {

			// Get sample
			sample  = event.sample(x);

			if (mux_channels) channel = chanMap[sample->channel()];
			else channel = sample->channel();

			if (flip_channels)
				channel = 127 - channel;

			channel += sample->apv()*128;

			//if (eventCount==0) printf("channel %d\n",channel);

			if ( channel >= (5 * 128) ) {
				cout << "Channel " << dec << channel << " out of range" << endl;
				cout << "Apv = " << dec << sample->apv() << endl;
				cout << "Chan = " << dec << sample->channel() << endl;
			}

			// Filter APVs
			if ( eventCount >= 20 ) {
				channelAvg[channel] = 0;
				for ( y=0; y < 6; y++ ) {
					value = sample->value(y);
					channelAvg[channel]+=value;

					//vhigh = (value << 1) & 0x2AAA;
					//vlow  = (value >> 1) & 0x1555;
					//value = vlow | vhigh;

					histAll[y]->Fill(value,channel);
					histAll[6]->Fill(value,channel);
					allSamples[channel][y][value]++;
					allSamples[channel][6][value]++;

					if ( value < histMin[channel] ) histMin[channel] = value;
					if ( value > histMax[channel] ) histMax[channel] = value;
					if ( value < hybridMin ) hybridMin = value;
					if ( value > hybridMax ) hybridMax = value;
				}
				channelAvg[channel]/=6.0;
				//channelAvg[channel] = sample->value(1);

				channelCount[channel]++;
				channelActive[channel] = true;
			}
		}
		if (!skip_corr)
			for (int i=0;i<640;i++) if (channelActive[i])
			{
				double delta = channelAvg[i]-channelMean[i];
				if (channelCount[i]==1)
				{
					channelMean[i] = channelAvg[i];
				}
				else
				{
					channelMean[i] += delta/channelCount[i];
				}
				channelVariance[i] += delta*(channelAvg[i]-channelMean[i]);
				for (int j=0;j<i;j++) if (channelActive[j])
				{
					channelCovar[i][j] += (channelAvg[i]-channelMean[i])*(channelAvg[j]-channelMean[j]);
					//if (i==corr1&&j==corr2) corrHist->Fill(channelAvg[i],channelAvg[j]);
				}
			}
		eventCount++;

	} while ( dataRead.next(&event));
	dataRead.close();

	int deadAPV = -1;
	for (int i=0;i<640;i++)
	{
		if (channelCount[i])
		{
			if (i/128==deadAPV)
			{
				printf("Counted %d events on channel %d, even though we thought APV %d was dead\n",channelCount[i],i,deadAPV);
			}
			if (channelCount[i]!=(int)eventCount-20)
			{
				printf("Counted %d events for channel %d; expected %d\n",channelCount[i],i,eventCount-20);
			}
			if (!skip_corr)
			{
				channelVariance[i]/=channelCount[i];
				for (int j=0;j<i;j++) if (channelCount[j])
				{
					channelCovar[i][j]/=eventCount-20;
					channelCovar[i][j]/=sqrt(channelVariance[i]);
					channelCovar[i][j]/=sqrt(channelVariance[j]);
					//printf("%d, %d, %f\n",i,j,channelCovar[i][j]);
				}
			}
		}
		else
		{
			if (i/128!=deadAPV)
			{
				deadAPV = i/128;
				printf("No events on channel %d; assuming APV %d is dead\n",i,deadAPV);
			}
		}
	}

	for (channel = 0; channel < 640; channel++) {
		int count;
		if (histMin[channel]<=histMax[channel])
		{
			grChan[ni]  = channel;
			outfile <<channel<<"\t";
			for (int i=0;i<7;i++)
			{
				doStats_mean(16384,histMin[channel],histMax[channel],allSamples[channel][i],count,grMean[i][ni],grSigma[i][ni]);
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

	if (!skip_corr)
	{
		//c1->SetLogz();
		TH2F *channelCorr = new TH2F("channelCorr","Channel correlation - positive correlations",640,-0.5,639.5,640,-0.5,639.5);
		channelCorr->SetStats(kFALSE);
		channelCorr->SetNdivisions(0,"XY");
		for (int i=0;i<640;i++) for (int j=0;j<i;j++) if (channelCovar[i][j]>0)
		{
			channelCorr->Fill(i,j,channelCovar[i][j]);
			channelCorr->Fill(j,i,channelCovar[i][j]);
		}
		channelCorr->Draw("colz");
		sprintf(name,"%s_corr_pos.pdf",inname.Data());
		c1->SaveAs(name);

		channelCorr->Reset();
		channelCorr->SetTitle("Channel correlation - negative correlations");
		for (int i=0;i<640;i++) for (int j=0;j<i;j++) if (channelCovar[i][j]<0)
		{
			channelCorr->Fill(i,j,-1*channelCovar[i][j]);
			channelCorr->Fill(j,i,-1*channelCovar[i][j]);
		}
		channelCorr->Draw("colz");
		sprintf(name,"%s_corr_neg.pdf",inname.Data());
		c1->SaveAs(name);
		//c1->SetLogz(0);

		c1->Clear();
		mg = new TMultiGraph();
		double yi[6][640], xi[6][640];
		for (int i=0;i<5;i++) 
		{
			for (int j=0;j<127;j++)
			{
				yi[i][j] = channelCovar[i*128+j+1][i*128+j];
				xi[i][j] = i*128+j+0.5;
			}
			graph[i] = new TGraph(127,xi[i],yi[i]);
			graph[i]->SetMarkerColor(i+2);
			mg->Add(graph[i]);
		}
		for (int i=1;i<5;i++)
		{
			yi[5][i-1] = channelCovar[i*128][i*128-1];
			xi[5][i-1] = i*128-0.5;
		}
		graph[5] = new TGraph(4,xi[5],yi[5]);
		graph[5]->SetMarkerColor(1);
		mg->Add(graph[5]);

		mg->SetTitle("Adjacent channel correlations");
		mg->Draw("a*");
		//mg->GetXaxis()->SetRangeUser(0,640);
		sprintf(name,"%s_corr_adj.png",inname.Data());
		c1->SaveAs(name);
		for (int i=0;i<6;i++) delete graph[i];
		delete mg;

		//printf("correlation %f %f\n",corrHist->GetCorrelationFactor(),channelCovar[corr1][corr2]);
	}

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

	c1->Clear();
	mg = new TMultiGraph();
	double grShift[640];
	for (int i=0;i<6;i++)
	{
		for (int n=0;n<ni;n++) grShift[n]=grMean[i][n]-grMean[6][n];
		graph[i] = new TGraph(ni,grChan,grShift);
		graph[i]->SetMarkerColor(i+1);
		mg->Add(graph[i]);
	}
	mg->SetTitle("Sample-to-sample shift;Channel;ADC counts");
	//mg->GetXaxis()->SetRangeUser(0,640);
	mg->Draw("a*");
	sprintf(name,"%s_base_shift.png",inname.Data());
	c1->SaveAs(name);
	for (int i=0;i<6;i++)
		delete graph[i];
	delete mg;

	// Start X-Windows
	//theApp.Run();

	// Close file
	outfile.close();
	return(0);
}

