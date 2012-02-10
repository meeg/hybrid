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
	double          calMean[640] = {0.0};
	double          calSigma[640] = {1.0};
	double calTp[2][640] = {{0.0}};
	double calA[2][640] = {{0.0}};
	double calT0[2][640] = {{0.0}};
	double calChisq[2][640] = {{0.0}};
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
	TH1F *histT0[2][640];
	TH1F *histA[2][640];
	TH1F *histT0_err[2][640];
	TH1F *histA_err[2][640];
	TH2F *histChiProb[2];

	initChan();

	//gStyle->SetOptStat(kFALSE);
	gStyle->SetPalette(1,0);

	// Start X11 view
	//TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc != 4 && argc != 5 ) {
		cout << "Usage: meeg_t0res data_file baseline_cal tp_cal [cal_group]\n";
		return(1);
	}

	int cal_grp = -1;
	if (argc==5)
		cal_grp=atoi(argv[4]);

	// 2d histogram
	for (int sgn=0;sgn<2;sgn++)
	{
		sprintf(name,"Chisq_Prob_%s",sgn?"Neg":"Pos");
		if (cal_grp==-1)
			histChiProb[sgn] = new TH2F(name,name,640,0,640,100,0,1.0);
		else
			histChiProb[sgn] = new TH2F(name,name,80,0,640,100,0,1.0);
	}

	for (int n=0;n<(cal_grp==-1?640:80);n++) {
		channel = cal_grp==-1?n:8*n+cal_grp;
		grChan[n]=channel;
		sprintf(name,"%i",channel);
		histMin[n] = 16384;
		histMax[n] = 0;
		for (int sgn=0;sgn<2;sgn++)
		{
			sprintf(name,"T0_%s_%i",sgn?"neg":"pos",channel);
			histT0[sgn][n] = new TH1F(name,name,1000,0,6*24.0);
			sprintf(name,"T0err_%s_%i",sgn?"neg":"pos",channel);
			histT0_err[sgn][n] = new TH1F(name,name,1000,0,10.0);
			sprintf(name,"A_%s_%i",sgn?"neg":"pos",channel);
			histA[sgn][n] = new TH1F(name,name,1000,0,2000.0);
			sprintf(name,"Aerr_%s_%i",sgn?"neg":"pos",channel);
			histA_err[sgn][n] = new TH1F(name,name,1000,0,100.0);
		}
	}

	// Attempt to open data file
	if ( ! dataRead.open(argv[1]) ) return(2);

	TString inname=argv[1];

	inname.ReplaceAll(".bin","");
	if (inname.Contains('/')) {
		inname.Remove(0,inname.Last('/')+1);
	}
	ifstream calfile;

	for (int sgn=0;sgn<2;sgn++)
	{
		sprintf(name,"%s_%s",argv[3],sgn?"neg":"pos");
		cout << "Reading Tp calibration from "<<name<<endl;
		calfile.open(name);
		while (!calfile.eof()) {
			calfile >> channel;
			calfile >> calA[sgn][channel];
			calfile >> calT0[sgn][channel];
			calfile >> calTp[sgn][channel];
			calfile >> calChisq[sgn][channel];

		}
		calfile.close();
	}

	cout << "Reading baseline calibration from " << argv[2] << endl;
	calfile.open(argv[2]);
	while (!calfile.eof()) {
		calfile >> channel;
		calfile >> calMean[channel];
		calfile >> calSigma[channel];
	}
	calfile.close();

	ShapingCurve *myShape[2][640];
	AnalyticFitter *myFitter[2][640];

	for (int n=0;n<(cal_grp==-1?640:80);n++) for (int sgn=0;sgn<2;sgn++) {
		channel = cal_grp==-1?n:8*n+cal_grp;
		//printf("%d, %d, %d, %f, %f\n",sgn, n, channel, calTp[sgn][channel], calSigma[channel]);
		myShape[sgn][n] = new ShapingCurve(calTp[sgn][channel]);
		myFitter[sgn][n] = new AnalyticFitter(myShape[sgn][n],6,1,1.0);
		myFitter[sgn][n]->setSigmaNoise(calSigma[channel]);
	}
	double temp_samples[6] = {0.0, 1.0, 2.0, 2.0, 1.0, 1.0};
	Samples *temp_mySamples = new Samples(6,24.0);
	temp_mySamples->readEvent(temp_samples,0.0);
	/*
	   myFitter[0][10]->setVerbosity(3);
	   myFitter[0][10]->readSamples(temp_mySamples);
	   myFitter[0][10]->doFit();
	   myFitter[0][10]->print_fit();
	   ShapingCurve *tempShape = new ShapingCurve(35.0);
	   AnalyticFitter *tempFitter = new AnalyticFitter(tempShape,6,1,1.0);
	   tempFitter->setSigmaNoise(0.1);
	   tempFitter->setVerbosity(3);
	   tempFitter->readSamples(temp_mySamples);
	   tempFitter->doFit();
	   tempFitter->print_fit();
	   return(0);
	   */
	//	cout << "Writing fit status to " << inname+".fits" << endl;
	//	ofstream fitfile;
	//	fitfile.open(inname+".fits");

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

			if (cal_grp!=-1 && (channel-cal_grp)%8!=0) continue;
			int n = cal_grp==-1?channel:channel/8;
			// Filter APVs
			if ( eventCount > 20 ) {

				sum = 0;
				for ( y=0; y < 6; y++ ) {
					value = sample->value(y);

					//vhigh = (value << 1) & 0x2AAA;
					//vlow  = (value >> 1) & 0x1555;
					//value = vlow | vhigh;

					//histAll->Fill(value,channel);

					if ( value < histMin[n] ) histMin[n] = value;
					if ( value > histMax[n] ) histMax[n] = value;
					samples[y] = value;
					samples[y] -= calMean[channel];
					sum+=samples[y];
				}
				if (sum<0)
					for ( y=0; y < 6; y++ ) {
						samples[y]*=-1;
					}
				if (abs(sum)>5*calSigma[channel])
				{
					int sgn = sum>0?0:1;
			//		cout<<sum<<"\t"<<sgn<<endl;
			//		printf("%d, %d, %d\n",sgn, n, channel);
					mySamples->readEvent(samples,0.0);
			//		mySamples->print();
					myFitter[sgn][n]->readSamples(mySamples);
					myFitter[sgn][n]->doFit();
			//		if (sgn==1 && n==0) {
			//			printf("meeg\n");
			//			myFitter[1][0]->doFit();
			//		}
			//		myFitter[sgn][n]->print_fit();
			//		myFitter[sgn][n]->printResiduals();
					myFitter[sgn][n]->getFitPar(fit_par);
					myFitter[sgn][n]->getFitErr(fit_err);
					chisq = myFitter[sgn][n]->getChisq(fit_par);
					dof = myFitter[sgn][n]->getDOF();
					histT0[sgn][n]->Fill(fit_par[0]);
					histT0_err[sgn][n]->Fill(fit_err[0]);
					histA[sgn][n]->Fill(fit_par[1]);
					histA_err[sgn][n]->Fill(fit_err[1]);
					chiprob = TMath::Prob(chisq,dof);
					//cout<<"Channel "<<channel << ", T0 " << fit_par[0] <<", A " << fit_par[1] << ", Fit chisq " << chisq << ", DOF " << dof << ", prob " << chiprob << endl;
					histChiProb[sgn]->Fill(channel,chiprob);
				}
				else
				{
					//					fitfile << "Pulse below threshold on channel "<<channel<<endl;
				}
			}
		}
		eventCount++;

	}

	double grT0[2][640], grT0_sigma[2][640], grT0_err[2][640];
	double grA[2][640], grA_sigma[2][640], grA_err[2][640];
	for (int n=0;n<(cal_grp==-1?640:80);n++) for (int sgn=0;sgn<2;sgn++) {
		channel = cal_grp==-1?n:8*n+cal_grp;
		histT0[sgn][n]->Fit("gaus","Q0");
		grT0[sgn][n] = histT0[sgn][n]->GetFunction("gaus")->GetParameter(1);
		grT0_sigma[sgn][n] = histT0[sgn][n]->GetFunction("gaus")->GetParameter(2);
		histT0_err[sgn][n]->Fit("gaus","Q0");
		grT0_err[sgn][n] = histT0_err[sgn][n]->GetFunction("gaus")->GetParameter(1);
		histA[sgn][n]->Fit("gaus","Q0");
		grA[sgn][n] = histA[sgn][n]->GetFunction("gaus")->GetParameter(1);
		grA_sigma[sgn][n] = histA[sgn][n]->GetFunction("gaus")->GetParameter(2);
		histA_err[sgn][n]->Fit("gaus","Q0");
		grA_err[sgn][n] = histA_err[sgn][n]->GetFunction("gaus")->GetParameter(1);
		outfile[sgn] <<channel<<"\t"<<grT0[sgn][n]<<"\t\t"<<grT0_sigma[sgn][n]<<"\t\t"<<grT0_err[sgn][n]<<"\t\t";
		outfile[sgn]<<grA[sgn][n]<<"\t\t"<<grA_sigma[sgn][n]<<"\t\t"<<grA_err[sgn][n]<<endl;     
	}

	c1 = new TCanvas("c1","c1",1200,900);
	//c1->SetLogz();
	TGraph * graph;
	for (int sgn=0;sgn<2;sgn++)
	{
		histChiProb[sgn]->Draw("colz");
		sprintf(name,"%s_t0_chiprob_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grT0[sgn]);
		sprintf(name,"T0_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->Draw("a*");
		sprintf(name,"%s_t0_T0_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grT0_err[sgn]);
		sprintf(name,"T0_err_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->Draw("a*");
		sprintf(name,"%s_t0_T0_err_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grT0_sigma[sgn]);
		sprintf(name,"T0_sigma_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->Draw("a*");
		sprintf(name,"%s_t0_T0_sigma_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grA[sgn]);
		sprintf(name,"A_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->Draw("a*");
		sprintf(name,"%s_t0_A_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grA_err[sgn]);
		sprintf(name,"A_err_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->Draw("a*");
		sprintf(name,"%s_t0_A_err_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(cal_grp==-1?640:80,grChan,grA_sigma[sgn]);
		sprintf(name,"A_sigma_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->Draw("a*");
		sprintf(name,"%s_t0_A_sigma_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

	}
	/*
	   c1->Clear();
	   histA[304][0]->Draw();
	   c1->SaveAs("histA.png");
	   c1->Clear();
	   histA_err[304][0]->Draw();
	   c1->SaveAs("histA_err.png");
	   c1->Clear();
	   histT0[304][0]->Draw();
	   c1->SaveAs("histT0.png");
	   c1->Clear();
	   histT0_err[304][0]->Draw();
	   c1->SaveAs("histT0_err.png");
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
	//	theApp.Run();

	// Close file
	dataRead.close();
	//	fitfile.close();
	outfile[0].close();
	outfile[1].close();
	return(0);
}

