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
#include <TStyle.h>
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
#include "ShapingCurve.hh"
#include "Samples.hh"
#include "Fitter.hh"
#include "AnalyticFitter.hh"
#include "LinFitter.hh"
#include <TMath.h>
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
	//TH2F            *histAll;
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
	TH1F *histT0[640][2];
	TH1F *histA[640][2];
	TH1F *histT0_err[640][2];
	TH1F *histA_err[640][2];
	TH2F *histChiProb[2];

	initChan();

	//gStyle->SetOptStat(kFALSE);
	gStyle->SetPalette(1,0);

	// Start X11 view
	//TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc != 3 ) {
		cout << "Usage: meeg_t0res data_file baseline_cal\n";
		return(1);
	}

	// 2d histogram
	histChiProb[0] = new TH2F("Chisq_Prob_Pos","Chisq_Prob_Pos",640,0,640,200,0,1.0);
	histChiProb[1] = new TH2F("Chisq_Prob_Neg","Chisq_Prob_Neg",640,0,640,200,0,1.0);

	ShapingCurve *myShape[640][2];
	Fitter *myFitter[640][2];

	for (channel=0; channel < 640; channel++) {
		grChan[channel]=channel;
		sprintf(name,"%i",channel);
		histMin[channel] = 16384;
		histMax[channel] = 0;
		for (int sgn=0;sgn<2;sgn++)
		{
			sprintf(name,"T0_%s_%i",sgn?"neg":"pos",channel);
			histT0[channel][sgn] = new TH1F(name,name,1000,0,6*24.0);
			sprintf(name,"T0err_%s_%i",sgn?"neg":"pos",channel);
			histT0_err[channel][sgn] = new TH1F(name,name,1000,0,10.0);
			sprintf(name,"A_%s_%i",sgn?"neg":"pos",channel);
			histA[channel][sgn] = new TH1F(name,name,1000,0,1000.0);
			sprintf(name,"Aerr_%s_%i",sgn?"neg":"pos",channel);
			histA_err[channel][sgn] = new TH1F(name,name,1000,0,100.0);
			myShape[channel][sgn] = new ShapingCurve(35.0);
			myFitter[channel][sgn] = new AnalyticFitter(myShape[channel][sgn],6,1,1.0);
		}
	}

	// Attempt to open data file
	if ( ! dataRead.open(argv[1]) ) return(2);

	TString inname=argv[1];

	inname.ReplaceAll(".bin","");
	if (inname.Contains('/')) {
		inname.Remove(0,inname.Last('/')+1);
	}
	cout << "Reading calibration from " << argv[2] << endl;
	ifstream calfile;
	calfile.open(argv[2]);

	while (!calfile.eof()) {
		calfile >> channel;
		calfile >> grMean[channel];
		calfile >> grSigma[channel];
		myFitter[channel][0]->setSigmaNoise(grSigma[channel]);
		myFitter[channel][1]->setSigmaNoise(grSigma[channel]);
	}
	calfile.close();

	cout << "Writing fit status to " << inname+".fits" << endl;
	ofstream fitfile;
	fitfile.open(inname+".fits");

	ofstream outfile[2];
	cout << "Writing T0 calibration to " << inname+".t0_pos" << endl;
	outfile[0].open(inname+".t0_pos");
	cout << "Writing T0 calibration to " << inname+".t0_neg" << endl;
	outfile[1].open(inname+".t0_neg");

	double samples[6];
	Samples *mySamples = new Samples(6,24.0);
	double fit_par[2], fit_err[2], chisq, chiprob;
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

					if ( value < histMin[channel] ) histMin[channel] = value;
					if ( value > histMax[channel] ) histMax[channel] = value;
					samples[y] = value - grMean[channel];
					sum+=samples[y];
				}
				if (sum<0)
					for ( y=0; y < 6; y++ ) {
						samples[y]*=-1;
					}
				if (abs(sum)>5*grSigma[channel])
				{
					int sgn = sum>0?0:1;
					mySamples->readEvent(samples,0.0);
					myFitter[channel][sgn]->readSamples(mySamples);
					myFitter[channel][sgn]->doFit();
					myFitter[channel][sgn]->getFitPar(fit_par);
					myFitter[channel][sgn]->getFitErr(fit_err);
					chisq = myFitter[channel][sgn]->getChisq(fit_par);
					dof = myFitter[channel][sgn]->getDOF();
					histT0[channel][sgn]->Fill(fit_par[0]);
					histT0_err[channel][sgn]->Fill(fit_err[0]);
					histA[channel][sgn]->Fill(fit_par[1]);
					histA_err[channel][sgn]->Fill(fit_err[1]);
					chiprob = TMath::Prob(chisq,dof);
					fitfile<<"Channel "<<channel << ", T0 " << fit_par[0] <<", A " << fit_par[1] << ", Fit chisq " << chisq << ", DOF " << dof << ", prob " << chiprob << endl;
					histChiProb[sgn]->Fill(channel,chiprob);
				}
				else
				{
					fitfile << "Pulse below threshold on channel "<<channel<<endl;
				}
			}
		}
		eventCount++;

	}

	double grT0[640][2], grT0_sigma[640][2], grT0_err[640][2];
	double grA[640][2], grA_sigma[640][2], grA_err[640][2];
	for(channel = 0; channel < 640; channel++) for (int sgn=0;sgn<2;sgn++) {
		histT0[channel][sgn]->Fit("gaus","Q0");
		grT0[channel][sgn] = histT0[channel][sgn]->GetFunction("gaus")->GetParameter(1);
		grT0_sigma[channel][sgn] = histT0[channel][sgn]->GetFunction("gaus")->GetParameter(2);
		histT0_err[channel][sgn]->Fit("gaus","Q0");
		grT0_err[channel][sgn] = histT0_err[channel][sgn]->GetFunction("gaus")->GetParameter(1);
		histA[channel][sgn]->Fit("gaus","Q0");
		grA[channel][sgn] = histA[channel][sgn]->GetFunction("gaus")->GetParameter(1);
		grA_sigma[channel][sgn] = histA[channel][sgn]->GetFunction("gaus")->GetParameter(2);
		histA_err[channel][sgn]->Fit("gaus","Q0");
		grA_err[channel][sgn] = histA_err[channel][sgn]->GetFunction("gaus")->GetParameter(1);
		outfile[sgn] <<channel<<"\t"<<grT0[channel][sgn]<<"\t\t"<<grT0_sigma[channel][sgn]<<"\t\t"<<grT0_err[channel][sgn]<<"\t\t";
		outfile[sgn]<<-1.0*grA[channel][sgn]<<"\t\t"<<grA_sigma[channel][sgn]<<"\t\t"<<grA_err[channel][sgn]<<endl;     
	}

	c1 = new TCanvas("c1","c1",1200,900);
	c1->SetLogz();
	histChiProb[0]->Draw("colz");
	sprintf(name,"%s_chiprob_pos.png",inname.Data());
	c1->SaveAs(name);

	histChiProb[1]->Draw("colz");
	sprintf(name,"%s_chiprob_neg.png",inname.Data());
	c1->SaveAs(name);
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
	//	theApp.Run();

	// Close file
	dataRead.close();
	outfile[0].close();
	outfile[1].close();
	return(0);
}

