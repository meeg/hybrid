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
#include <TMath.h>
#include <TGraphErrors.h>
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
	int grChan[640];
	TH2F            *histSamples[640][2];
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
	double grTp[640][2] = {{0.0}};
	double grA[640][2] = {{0.0}};
	double grT0[640][2] = {{0.0}};
	double grChisq[640][2] = {{0.0}};

	initChan();

	//gStyle->SetOptStat(kFALSE);
	gStyle->SetPalette(1,0);

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
		grChan[channel]=channel;
		for (int sgn=0;sgn<2;sgn++)
		{
		sprintf(name,"samples_%s_%i",sgn?"neg":"pos",channel);
		histSamples[channel][sgn] = new TH2F(name,name,6,-0.5*24.0,5.5*24.0,16384,-0.5,16383.5);
		}
		histMin[channel] = 16384;
		histMax[channel] = 0;

		/*
		   sprintf(name,"T0_%i",channel);
		   histT0[channel] = new TH1F(name,name,1000,0,6*24.0);
		   sprintf(name,"T0err_%i",channel);
		   histT0_err[channel] = new TH1F(name,name,1000,0,10.0);
		   sprintf(name,"A_%i",channel);
		   histA[channel] = new TH1F(name,name,1000,0,16384);
		   sprintf(name,"Aerr_%i",channel);
		   histA_err[channel] = new TH1F(name,name,1000,0,1000);
		   */
	}

	// Attempt to open data file
	if ( ! dataRead.open(argv[1]) ) return(2);

	TString inname=argv[1];

	inname.ReplaceAll(".bin","");
	if (inname.Contains('/')) {
		inname.Remove(0,inname.Last('/')+1);
	}
	/*
	   cout << "Reading calibration from " << inname+".calib" << endl;
	   ifstream calfile;
	   calfile.open(inname+".calib");

	   while (!calfile.eof()) {
	   calfile >> channel;
	   calfile >> grMean[channel];
	   calfile >> grSigma[channel];
	   myFitter[channel]->setSigmaNoise(grSigma[channel]);
	   }
	   calfile.close();
	   */

	ofstream outfile[2];
	cout << "Writing Tp calibration to " << inname+".tp_pos" << endl;
	outfile[0].open(inname+".tp_pos");
	cout << "Writing Tp calibration to " << inname+".tp_neg" << endl;
	outfile[1].open(inname+".tp_neg");



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
					sum+=value;
					sum-=sample->value(0);
				}
				int sgn = sum>0?0:1;
				for ( y=0; y < 6; y++ ) {
					histSamples[channel][sgn]->Fill(y*24.0,sample->value(y));
				}
				//outfile<<"T0 " << fit_par[0] <<", A " << fit_par[1] << "Fit chisq " << chisq << ", DOF " << dof << ", prob " << TMath::Prob(chisq,dof) << endl;
			}
		}
		eventCount++;

	}

	double yi[6], ey[6], ti[6];
	TGraphErrors *fitcurve;
	TF1 *shapingFunction = new TF1("Shaping Function",
			"[3]+[0]*((x-[1])/[2])*exp(1-((x-[1])/[2]))",0, 6*24);
	c1 = new TCanvas("c1","c1",1200,900);

	for(channel = 0; channel < 640; channel++) for (int sgn=0;sgn<2;sgn++) {
		//cout<<channel<<endl;
		histSamples[channel][sgn]->GetYaxis()->SetRangeUser(histMin[channel],histMax[channel]);
		if (histSamples[channel][sgn]->GetEntries()>20)
		{
			c1->Clear();
			//histSamples[channel]->Print();
			histSamples[channel][sgn]->Draw("colz");
			histSamples[channel][sgn]->FitSlicesY();
			//gDirectory->ls();

			sprintf(name,"samples_%s_%d_1",sgn?"neg":"pos",channel);
			TH1F *means = (TH1F*)gDirectory->Get(name);
			sprintf(name,"samples_%s_%d_2",sgn?"neg":"pos",channel);
			TH1F *sigmas = (TH1F*)gDirectory->Get(name);
			for (int i=0;i<6;i++)
			{
				yi[i] = means->GetBinContent(i+1);
				ey[i] = sigmas->GetBinContent(i+1);
				ti[i]= i*24.0;
			}
			fitcurve = new TGraphErrors(6,ti,yi,NULL,ey);
			fitcurve->Draw();
			shapingFunction->SetParameter(0,TMath::MaxElement(6,yi)-yi[0]);
			shapingFunction->SetParameter(1,12.0);
			shapingFunction->SetParameter(2,50.0);
			shapingFunction->FixParameter(3,yi[0]);
			if (fitcurve->Fit(shapingFunction,"Q","",24.0,6*24.0)==0)
			{
				grA[channel][sgn]=shapingFunction->GetParameter(0);
				grT0[channel][sgn]=shapingFunction->GetParameter(1);
				grTp[channel][sgn]=shapingFunction->GetParameter(2);
				grChisq[channel][sgn]=shapingFunction->GetChisquare();
			}
			else
				cout<<"Bad fit for positive pulses, channel "<<channel<<endl;
			sprintf(name,"%s_curve_%s_%i.png",inname.Data(),sgn?"neg":"pos",channel);
			c1->SaveAs(name);

			outfile[sgn] <<channel<<"\t"<<grA[channel][sgn]<<"\t"<<grT0[channel][sgn]<<"\t"<<grTp[channel][sgn]<<"\t"<<grChisq[channel][sgn]<<endl;

		}
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
	outfile[0].close();
	outfile[1].close();
	return(0);
}

