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
#include <DataReadEvio.h>
#include <unistd.h>
using namespace std;

//#define corr1 599
//#define corr1 604
//#define corr2 500

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
        bool debug = false;
	bool flip_channels = true;
	bool mux_channels = false;
	bool skip_corr = false;
	bool read_temp = false;
	bool evio_format = false;
	int fpga = -1;
	int hybrid = -1;
	int num_events = -1;
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
	//TH2S *corrHist;
	//corrHist = new TH2S("corrHist","Channel correlation",16384,-0.5,16383.5,16384,-0.5,16383.5);

	double          histMin[640];
	double          histMax[640];
	uint hybridMin, hybridMax;
	//TGraph          *sigma;
	int ni = 0;
	double          grChan[640];
	double          grMean[7][640];
	double          grSigma[7][640];
	DataRead        *dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	uint            value;
	uint            channel;
	int            eventCount;
	int runCount;
	TString inname;
	TString outdir;
	char            name[200];
	char title[200];
	TGraph          *graph[7];
	TMultiGraph *mg;

	while ((c = getopt(argc,argv,"ho:nmctH:F:e:Ed")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-d: turn on debug\n");
				printf("-o: use specified output filename\n");
				printf("-n: DAQ (Ryan's) channel numbering\n");
				printf("-m: number channels in raw mux order\n");
				printf("-c: don't compute correlations\n");
				printf("-t: print temperature\n");
				printf("-F: use only specified FPGA\n");
				printf("-H: use only specified hybrid\n");
				printf("-e: stop after specified number of events\n");
				printf("-E: use EVIO file format\n");
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
			case 'c':
				skip_corr = true;
				break;
			case 'm':
				mux_channels = true;
				break;
			case 't':
				read_temp = true;
				break;
			case 'F':
				fpga = atoi(optarg);
				break;
			case 'H':
				hybrid = atoi(optarg);
				break;
			case 'e':
				num_events = atoi(optarg);
				break;
			case 'E':
				evio_format = true;
				break;
			case 'd':
				debug = true;
				break;
			case '?':
				printf("Invalid option or missing option argument; -h to list options\n");
				return(1);
			default:
				abort();
		}

	if (evio_format)
		dataRead = new DataReadEvio();
	else 
		dataRead = new DataRead();

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
	if ( ! dataRead->open(argv[optind]) ) return(2);

	TString confname=argv[optind];
	confname.ReplaceAll(".bin","");
	confname.Append(".conf");
	if (confname.Contains('/')) {
		confname.Remove(0,confname.Last('/')+1);
	}

	ofstream outconfig;
	cout << "Writing configuration to " <<outdir<<confname << endl;
	outconfig.open(outdir+confname);

	dataRead->next(&event);
	dataRead->dumpConfig(outconfig);
	outconfig.close();
	//dataRead.dumpStatus();

	runCount = atoi(dataRead->getConfig("RunCount").c_str());
	int max_count = runCount==0 ? 10000 : runCount;
	double *apv_means[5];
	for (int i=0;i<5;i++) apv_means[i] = new double[max_count];
	double *moving_ti = new double[max_count];
	/*
	   double *moving_yi = new double[runCount];
	   double *moving_yi2 = new double[runCount];
	   */

	if(debug) printf("%d events expected from config\n",runCount);


	// Process each event
	eventCount = 0;

	do {
	        if(debug) printf("Event %d\n",eventCount);

	        if(debug) printf("fpga %d\n",event.fpgaAddress());
		if (event.fpgaAddress()==7) 
		{
			printf("not a data event\n");
			continue;
		}
		if (fpga!=-1 && ((int)event.fpgaAddress())!=fpga) continue;
		if(debug) cout<<"  fpga #"<<event.fpgaAddress()<<"; number of samples = "<<event.count() << " is" <<endl;
		if(debug) cout<<"  fpga #"<<event.fpgaAddress()<<"; number of samples = "<<event.count()<<endl;
		if (eventCount%1000==0) printf("Event %d\n",eventCount);
		if (num_events!=-1 && eventCount >= num_events) break;
		if (read_temp && !event.isTiFrame()) for (uint i=0;i<4;i++)
			if (event.temperature(i)!=0.0)
			{
				printf("Event %d, temperature #%d: %f\n",eventCount,i,event.temperature(i));
				read_temp = false;
			}
		if (eventCount<max_count) {
			moving_ti[eventCount] = eventCount;
			for (int i=0;i<5;i++) apv_means[i][eventCount] = 0.0;
		}
		for (int i=0;i<640;i++)
		{
			channelActive[i] = false;
		}

		for (x=0; x < event.count(); x++) {
			// Get sample
			sample  = event.sample(x);
			if(debug) printf("event %d\tx=%d\tF%d H%d A%d channel %d, samples:\t%d\t%d\t%d\t%d\t%d\t%d\n",eventCount,x,event.fpgaAddress(),sample->hybrid(),sample->apv(),sample->channel(),sample->value(0),sample->value(1),sample->value(2),sample->value(3),sample->value(4),sample->value(5));
			if (hybrid!=-1 && ((int)sample->hybrid())!=hybrid) continue;
			//printf("hybrid %d\n",sample->hybrid());

			if (mux_channels) channel = chanMap[sample->channel()];
			else channel = sample->channel();

			if (flip_channels)
				channel += (4-sample->apv())*128;
			else
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
			double mean = 0;
			for (y=0;y<6;y++) mean+=sample->value(y);
			mean/=6.0;
			if (eventCount<max_count) {
				apv_means[sample->apv()][eventCount] += mean;
			}
			/*
			   if (channel==corr1)
			   {
			   moving_yi[eventCount] = mean;
			   }
			   if (channel==corr2)
			   {
			   moving_yi2[eventCount] = mean;
			   }
			   */
		}
		if (eventCount<max_count) {
		for (int i=0;i<5;i++) apv_means[i][eventCount] /= 128;
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
					/*
					   if (i==corr1&&j==corr2)
					   {
					//if (channelAvg[i]<7620)
					corrHist->Fill(channelAvg[i],channelAvg[j]);
					//else
					//printf("event %d\n",eventCount);
					}
					*/
				}
			}
		eventCount++;

	} while ( dataRead->next(&event));
	dataRead->close();

	if (eventCount != runCount)
	{
		printf("ERROR: events read = %d, runCount = %d\n",eventCount, runCount);
	}

	/*
	   TGraph *movingGraph = new TGraph(eventCount,moving_ti,moving_yi);
	   TGraph *movingGraph2 = new TGraph(eventCount,moving_ti,moving_yi2);
	   movingGraph->SetMarkerColor(2);
	   mg = new TMultiGraph();
	   mg->Add(movingGraph);
	   mg->Add(movingGraph2);

	   mg->Draw("a*");
	   c1->SaveAs("meeg.png");
	   c1->Clear();
	   */

	mg = new TMultiGraph();
	for (int i=0;i<5;i++)
	{
		graph[i] = new TGraph(min(eventCount,max_count),moving_ti,apv_means[i]);
		graph[i]->SetMarkerColor(i+1);
		if (i==4) graph[i]->SetMarkerColor(6);
		graph[i]->SetMarkerStyle(20);
		graph[i]->SetMarkerSize(0.25);
		mg->Add(graph[i]);
	}
	mg->Draw("ap");
	sprintf(name,"%s_base_apvmeans.png",inname.Data());
	printf("%s\n",name);
	c1->SaveAs(name);
	c1->Clear();
	//for (int i=0;i<5;i++) delete graph[i];
	delete mg;
	for (int i=0;i<5;i++) delete[] apv_means[i];
	delete[] moving_ti;

	/*
	   c1->Clear();
	   corrHist->GetXaxis()->SetRangeUser(histMin[corr1],histMax[corr1]);
	   corrHist->GetYaxis()->SetRangeUser(histMin[corr2],histMax[corr2]);
	   corrHist->Draw("colz");
	   c1->SaveAs("meeg2.png");
	   */

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
				doStats_mean(16384,(int)histMin[channel],(int)histMax[channel],allSamples[channel][i],count,grMean[i][ni],grSigma[i][ni]);
				outfile<<grMean[i][ni]<<"\t"<<grSigma[i][ni]<<"\t";
			}
			outfile<<endl;
			ni++;
		}
	}


	histAll[6]->GetXaxis()->SetRangeUser(hybridMin,hybridMax);
	histAll[6]->Draw("colz");
	sprintf(name,"%s_base.png",inname.Data());
	c1->SaveAs(name);

	if (!skip_corr)
	{
		//c1->SetLogz();
		TH2F *channelCorr = new TH2F("channelCorr","Channel correlation - positive correlations;channel 1;channel 2",640,-0.5,639.5,640,-0.5,639.5);
		channelCorr->SetStats(kFALSE);
		channelCorr->SetNdivisions(0,"XY");
		for (int i=0;i<640;i++) for (int j=0;j<i;j++) if (channelCovar[i][j]>0)
		{
			channelCorr->Fill(i,j,channelCovar[i][j]);
			channelCorr->Fill(j,i,channelCovar[i][j]);
		}
		channelCorr->Draw("colz");
		sprintf(name,"%s_pos_corr.pdf",inname.Data());
		c1->SaveAs(name);
		sprintf(name,"%s_pos_corr.png",inname.Data());
		c1->SaveAs(name);

		channelCorr->Reset();
		channelCorr->SetTitle("Channel correlation - negative correlations;channel 1;channel 2");
		for (int i=0;i<640;i++) for (int j=0;j<i;j++) if (channelCovar[i][j]<0)
		{
			channelCorr->Fill(i,j,-1*channelCovar[i][j]);
			channelCorr->Fill(j,i,-1*channelCovar[i][j]);
		}
		channelCorr->Draw("colz");
		sprintf(name,"%s_neg_corr.pdf",inname.Data());
		c1->SaveAs(name);
		sprintf(name,"%s_neg_corr.png",inname.Data());
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
		//printf("correlation %f\n",channelCovar[corr1][corr2]);
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
	double max = 0;
	for (int i=0;i<7;i++)
		for (int j=0;j<ni;j++)
			if (grSigma[i][j]>max) max = grSigma[i][j];

	for (int i=0;i<7;i++)
	{
		graph[i] = new TGraph(ni,grChan,grSigma[i]);
		graph[i]->SetMarkerColor(i+2);
		mg->Add(graph[i]);
	}
	graph[6]->SetMarkerColor(1);
	mg->SetTitle("Noise;Channel;ADC counts");
	mg->Draw("a*");
	mg->GetYaxis()->SetRangeUser(0,1.2*max);
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
	delete dataRead;
	for (int i=0;i<640;i++) for (int j=0;j<7;j++)
	{
		delete[] allSamples[i][j];
		//for (int k=0;k<16384;k++) allSamples[i][j][k]=0;
	}
	return(0);
}

