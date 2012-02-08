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
#include <TApplication.h>
#include <TGraph.h>
//#include <TStyle.h>
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
#include "TMath.h"
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
	TCanvas         *c1, *c2, *c4, *c5;
	//TH2F            *histAll;
	TH2F            *histSamples[640];
	double          histMin[640];
	double          histMax[640];
	//TGraph          *mean;
	//TGraph          *sigma;
	double          grChan[640];
	double          grMean[640] = {0.0};
	double          grSigma[640] = {1.0};
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
	TH1F *histT0[640];
	TH1F *histA[640];
	TH1F *histT0_err[640];
	TH1F *histA_err[640];

	initChan();

	//gStyle->SetOptStat(kFALSE);

	// Start X11 view
	   //TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc != 2 ) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}

	// 2d histogram
	//histAll = new TH2F("Value_Hist_All","Value_Hist_All",16384,0,16384,640,0,640);

	for (channel=0; channel < 640; channel++) {
		sprintf(name,"%i",channel);
		histSamples[channel] = new TH2F(name,name,6,-0.5,5.5,16384,-0.5,16383.5);
		histMin[channel] = 16384;
		histMax[channel] = 0;

		sprintf(name,"T0_%i",channel);
		histT0[channel] = new TH1F(name,name,1000,0,6*24.0);
		sprintf(name,"T0err_%i",channel);
		histT0_err[channel] = new TH1F(name,name,1000,0,10.0);
		sprintf(name,"A_%i",channel);
		histA[channel] = new TH1F(name,name,1000,0,16384);
		sprintf(name,"Aerr_%i",channel);
		histA_err[channel] = new TH1F(name,name,1000,0,1000);
	}

	// Attempt to open data file
	if ( ! dataRead.open(argv[1]) ) return(2);

	TString inname=argv[1];

	inname.ReplaceAll(".bin","");
	if (inname.Contains('/')) {
		inname.Remove(0,inname.Last('/')+1);
	}
	cout << "Reading calibration from " << inname+".calib" << endl;
	ifstream calfile;
	calfile.open(inname+".calib");

	while (!calfile.eof()) {
		calfile >> channel;
		calfile >> grMean[channel];
		calfile >> grSigma[channel];
		//myFitter[channel]->setSigmaNoise(grSigma[channel]);
	}
	calfile.close();

	cout << "Writing calibration to " << inname+".calib2" << endl;
	ofstream outfile;
	outfile.open(inname+".calib2");

	double samples[6];
	double fit_par[2], fit_err[2], chisq;
	int dof;


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

				sum = 0;
				for ( y=0; y < 6; y++ ) {
					value = sample->value(y);

					//vhigh = (value << 1) & 0x2AAA;
					//vlow  = (value >> 1) & 0x1555;
					//value = vlow | vhigh;

					//histAll->Fill(value,channel);
					//histSng[channel]->Fill(value);

					if ( value < histMin[channel] ) histMin[channel] = value;
					if ( value > histMax[channel] ) histMax[channel] = value;
					samples[y] = value - grMean[channel];
					sum+=samples[y];
					histSamples[channel]->Fill(y,value);
				}
				if (sum<0)
					for ( y=0; y < 6; y++ ) {
						samples[y]*=-1;
					}
				//outfile<<"T0 " << fit_par[0] <<", A " << fit_par[1] << "Fit chisq " << chisq << ", DOF " << dof << ", prob " << TMath::Prob(chisq,dof) << endl;
			}
		}
		eventCount++;

	}

	double grT0[640], grT0_sigma[640], grT0_err[640];
	double grA[640], grA_sigma[640], grA_err[640];
	for(channel = 0; channel < 640; channel++) {
		c1 = new TCanvas("c1","c1");
		histSamples[channel]->Draw("colz");
		sprintf(name,"samples_%i.png",channel);
		c1->SaveAs(name);
		/*
		   histT0[channel]->Fit("gaus","Q0");
		   grT0[channel] = histT0[channel]->GetFunction("gaus")->GetParameter(1);
		   grT0_sigma[channel] = histT0[channel]->GetFunction("gaus")->GetParameter(2);
		   histT0_err[channel]->Fit("gaus","Q0");
		   grT0_err[channel] = histT0_err[channel]->GetFunction("gaus")->GetParameter(1);
		   histA[channel]->Fit("gaus","Q0");
		   grA[channel] = histA[channel]->GetFunction("gaus")->GetParameter(1);
		   grA_sigma[channel] = histA[channel]->GetFunction("gaus")->GetParameter(2);
		   histA_err[channel]->Fit("gaus","Q0");
		   grA_err[channel] = histA_err[channel]->GetFunction("gaus")->GetParameter(1);
		   outfile <<channel<<"\t"<<grT0[channel]<<"\t"<<grT0_sigma[channel]<<"\t"<<grT0_err[channel]<<"\t";
		   outfile<<grA[channel]<<"\t"<<grA_sigma[channel]<<"\t"<<grA_err[channel]<<endl;     
		   */
	}

	/*
	   c1 = new TCanvas("c1","c1");
	   c1->cd();
	   histAll->Draw("colz");
	   */

	/*
	   c2 = new TCanvas("c2","c2");
	//c2->Divide(12,11,0.01);

	histT0[300]->Draw();
	int base = 3 * 128;
	*/
	/*
	   for(channel = base; channel < (base+128); channel++) {
	   c2->cd((channel-base)+1);
	//	histSng[channel]->GetXaxis()->SetRangeUser(histMin[channel],histMax[channel]);
	//	histSng[channel]->Draw();
	histT0[channel]->Draw();
	}
	*/
	/*
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

