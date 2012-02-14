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
#include <TH2I.h>
#include <TH2D.h>
#include <TF1.h>
#include <TROOT.h>
#include <TCanvas.h>
#include <TApplication.h>
#include <TGraph.h>
#include <TStyle.h>
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
#include <TMath.h>
#include <TGraphErrors.h>
#include <unistd.h>
using namespace std;

#define SAMPLE_INTERVAL 25.0

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
	bool plot_tp_fits = false;
	bool plot_fit_results = false;
	int cal_grp = -1;
	TCanvas         *c1;
	//TH2I            *histAll;
	double grChan[640];
	TH2I            *histSamples[2][640];
	TH1D            *histSamples1D[2][640][6];
	double          histMin[640];
	double          histMax[640];
	//TGraph          *mean;
	//TGraph          *sigma;
	DataRead        dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	uint            value;
	uint            channel;
	uint            eventCount;
	double          sum;
	char            name[100];
	double grTp[2][640] = {{0.0}};
	double grA[2][640] = {{0.0}};
	double grT0[2][640] = {{0.0}};
	double grChisq[2][640] = {{0.0}};

	while ((c = getopt(argc,argv,"hfrg:")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-f: plot Tp fits for each channel\n");
				printf("-g: force use of specified cal group\n");
				printf("-r: plot fit results\n");
				return(0);
				break;
			case 'f':
				plot_tp_fits = true;
				break;
			case 'r':
				plot_fit_results = true;
				break;
			case 'g':
				cal_grp = atoi(optarg);
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
	//TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind != 1) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}


	// Attempt to open data file
	if ( ! dataRead.open(argv[optind]) ) return(2);

	dataRead.next(&event);
	dataRead.dumpConfig();
	dataRead.dumpStatus();

	if (cal_grp==-1)
	{
		cal_grp = atoi(dataRead.getConfig("cntrlFpga:hybrid:apv25:CalGroup").c_str());
		cout<<"Read calibration group "<<cal_grp<<" from data file"<<endl;
	}

	for (int n=0;n<80;n++) {
		channel = 8*n+cal_grp;
		grChan[n]=channel;
		for (int sgn=0;sgn<2;sgn++)
		{
			sprintf(name,"samples_%s_%i",sgn?"neg":"pos",channel);
			histSamples[sgn][n] = new TH2I(name,name,6,-0.5*SAMPLE_INTERVAL,5.5*SAMPLE_INTERVAL,16384,-0.5,16383.5);
		}
		histMin[n] = 16384;
		histMax[n] = 0;
	}

	TString inname=argv[optind];

	inname.ReplaceAll(".bin","");
	if (inname.Contains('/')) {
		inname.Remove(0,inname.Last('/')+1);
	}

	ofstream tpfile[2];
	cout << "Writing Tp calibration to " << inname+".tp_pos" << endl;
	tpfile[0].open(inname+".tp_pos");
	cout << "Writing Tp calibration to " << inname+".tp_neg" << endl;
	tpfile[1].open(inname+".tp_neg");

	ofstream shapefile[2];
	cout << "Writing pulse shape to " << inname+".shape_pos" << endl;
	shapefile[0].open(inname+".shape_pos");
	cout << "Writing pulse shape to " << inname+".shape_neg" << endl;
	shapefile[1].open(inname+".shape_neg");



	// Process each event
	eventCount = 0;

	do {
		for (x=0; x < event.count(); x++) {

			// Get sample
			sample  = event.sample(x);
			channel = (sample->apv() * 128) + sample->channel();

			if ( channel >= (5 * 128) ) {
				cout << "Channel " << dec << channel << " out of range" << endl;
				cout << "Apv = " << dec << sample->apv() << endl;
				cout << "Chan = " << dec << sample->channel() << endl;
			}

			if ((channel-cal_grp)%8!=0) continue;
			int n = channel/8;
			// Filter APVs
			if ( eventCount > 20 ) {

				sum = 0;
				for ( y=0; y < 6; y++ ) {
					value = sample->value(y);

					//vhigh = (value << 1) & 0x2AAA;
					//vlow  = (value >> 1) & 0x1555;
					//value = vlow | vhigh;

					//histAll->Fill(value,channel);
					//histSng[channel]->Fill(value);

					if ( value < histMin[n] ) histMin[n] = value;
					if ( value > histMax[n] ) histMax[n] = value;
					sum+=value;
				}
				sum-=6*sample->value(0);
				int sgn = sum>0?0:1;
				for ( y=0; y < 6; y++ ) {
					histSamples[sgn][n]->Fill(y*SAMPLE_INTERVAL,sample->value(y));
				}
				//tpfile<<"T0 " << fit_par[0] <<", A " << fit_par[1] << "Fit chisq " << chisq << ", DOF " << dof << ", prob " << TMath::Prob(chisq,dof) << endl;
			}
		}
		eventCount++;

	} while ( dataRead.next(&event) );

	double yi[6], ey[6], ti[6];
	TGraphErrors *fitcurve;
	TF1 *shapingFunction = new TF1("Shaping Function",
			"[3]+[0]*((x-[1])/[2])*exp(1-((x-[1])/[2]))",0, 6*24);
	c1 = new TCanvas("c1","c1",1200,900);

	for (int n=0;n<80;n++) for (int sgn=0;sgn<2;sgn++) {
		channel = 8*n+cal_grp;
		if (plot_tp_fits) histSamples[sgn][n]->GetYaxis()->SetRangeUser(histMin[n],histMax[n]);
		if (histSamples[sgn][n]->GetEntries()>20)
		{
			if (plot_tp_fits)
			{
				c1->Clear();
				histSamples[sgn][n]->Draw("colz");
			}
			//histSamples[sgn][n]->FitSlicesY();

			//sprintf(name,"samples_%s_%d_1",sgn?"neg":"pos",channel);
			//TH1F *means = (TH1F*)gDirectory->Get(name);
			//sprintf(name,"samples_%s_%d_2",sgn?"neg":"pos",channel);
			//TH1F *sigmas = (TH1F*)gDirectory->Get(name);
			shapefile[sgn]<<channel;
			for (int i=0;i<6;i++)
			{
				sprintf(name,"samples_%s_%i_%i",sgn?"neg":"pos",channel,i);
				histSamples1D[sgn][n][i] = histSamples[sgn][n]->ProjectionY(name,i+1,i+1);

				if (histSamples1D[sgn][n][i]->Fit("gaus","Q0")==0) {
					yi[i]  = histSamples1D[sgn][n][i]->GetFunction("gaus")->GetParameter(1);
					ey[i]  = histSamples1D[sgn][n][i]->GetFunction("gaus")->GetParError(1);
				}
				else {
					cout << "Could not fit channel " << channel << endl;
					yi[i]  = 0;
					ey[i] = 1;
				}

				//yi[i] = histSamples1D[sgn][n][i]->GetMean();
				//ey[i] = histSamples1D[sgn][n][i]->GetRMS();
				shapefile[sgn]<<"\t"<<yi[i];
				//yi[i] = means->GetBinContent(i+1);
				//ey[i] = sigmas->GetBinContent(i+1);
				ti[i]= i*SAMPLE_INTERVAL;
			}
			shapefile[sgn]<<endl;
			fitcurve = new TGraphErrors(6,ti,yi,NULL,ey);
			if (plot_tp_fits) fitcurve->Draw();
			shapingFunction->SetParameter(0,TMath::MaxElement(6,yi)-yi[0]);
			shapingFunction->SetParameter(1,12.0);
			shapingFunction->SetParameter(2,50.0);
			shapingFunction->FixParameter(3,yi[0]);
			if (fitcurve->Fit(shapingFunction,plot_tp_fits?"Q":"Q0","",SAMPLE_INTERVAL,6*SAMPLE_INTERVAL)==0)
			{
				grA[sgn][n]=shapingFunction->GetParameter(0);
				if (sgn==1) grA[sgn][n]*=-1;
				grT0[sgn][n]=shapingFunction->GetParameter(1);
				grTp[sgn][n]=shapingFunction->GetParameter(2);
				grChisq[sgn][n]=shapingFunction->GetChisquare();
			}
			else
			{
				cout<<"Bad fit for positive pulses, channel "<<channel<<endl;
			}
			if (plot_tp_fits)
			{
				sprintf(name,"%s_tp_%s_%i.png",inname.Data(),sgn?"neg":"pos",channel);
				c1->SaveAs(name);
			}
		}
		tpfile[sgn] <<channel<<"\t"<<grA[sgn][n]<<"\t"<<grT0[sgn][n]<<"\t"<<grTp[sgn][n]<<"\t"<<grChisq[sgn][n]<<endl;
	}

	TGraph * graph;
	if (plot_fit_results) for (int sgn=0;sgn<2;sgn++)
	{
		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grA[sgn]);
		sprintf(name,"A_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_A_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grT0[sgn]);
		sprintf(name,"T0_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_T0_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grTp[sgn]);
		sprintf(name,"Tp_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_Tp_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grChisq[sgn]);
		sprintf(name,"Chisq_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_Chisq_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);
	}

	// Start X-Windows
	//theApp.Run();

	// Close file
	dataRead.close();
	tpfile[0].close();
	tpfile[1].close();
	shapefile[0].close();
	shapefile[1].close();
	return(0);
}

