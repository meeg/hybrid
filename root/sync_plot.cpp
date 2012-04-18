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
#include <TCanvas.h>
#include <TMultiGraph.h>
#include <TApplication.h>
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
	TCanvas         *c1, *c2;
	TH1F            *hist[32];
	double          histMin[32];
	double          histMax[32];
	double          meanVal[32];
	double          plotX[32];
	ifstream        inFile;
	string          inLine;
	stringstream    inLineStream;
	string          inValue;
	stringstream    inValueStream;
	int             adcValue;
	uint            adcValid;
	uint            x;
	char    title[200];
	uint            idx;
	TGraph          *plot;
	double          value;
	double          sum;

	char c;
	bool no_gui = false;
	bool print_data = false;
	while ((c = getopt(argc,argv,"hgd")) !=-1)
		switch (c)
		{
			case 'g':
				no_gui = true;
				break;
			case 'd':
				print_data = true;
				break;
			case 'h':
				printf("-h: print this help\n");
				printf("-g: no GUI\n");
				return(0);
				break;
			case '?':
				printf("Invalid option or missing option argument; -h to list options\n");
				return(1);
			default:
				abort();
		}

	gStyle->SetOptStat(kFALSE);

	// Start X11 view
	//if (!no_gui)
	TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind < 1 ) {
		cout << "Usage: sync_plot files.csv\n";
		return(1);
	}

	for (x=0; x < 32; x++) {
		sprintf(title,"sample_%d",x);
		hist[x] = new TH1F(title,title,32768,-16383.5,16384.5);
		histMin[x] = 16384;
		histMax[x] = -16384;
	}

	while (optind<argc) {
		inFile.open(argv[optind]);

		cout << "Reading file " << argv[optind] << endl;
		optind++;

		// Skip first line 
		getline(inFile,inLine);

		idx = 100;
		int data[100];
		while ( getline(inFile,inLine) ) {
			inLineStream.clear();
			inLineStream.str(inLine);

			for (int i=0;i<100;i++)
			{
				getline(inLineStream,inValue,'\t');
				inValueStream.clear();
				inValueStream.str(inValue);
				inValueStream >> hex >> data[i];
				//printf("i = %d, val = %d\n",i,data[i]);
			}

			adcValue = data[67];
			adcValid = data[51];

			if (print_data) printf("adcValue = %d, adcValid = %d\n",adcValue,adcValid);
			if ( adcValid ) {
				if (print_data) cout << "Value=0x" << hex << adcValue << endl;

				if ( adcValue > 0x2000 ) idx = 0;

				if ( idx < 32 ) {
					if ( idx != 0 ) value = adcValue;
					else value = adcValue;
					if (print_data) cout << "Fill idx=" << dec << idx << " value=0x" << hex << value << endl;
					hist[idx]->Fill(value);
					if ( value < histMin[idx] ) histMin[idx] = value;
					if ( value > histMax[idx] ) histMax[idx] = value;
				}
				idx++;
			}
		}

		inFile.close();
	}

	if (!no_gui)
	{
	c1 = new TCanvas("c1","c1");
	c1->Divide(8,4,0.0125,0.0125);
	}

	double mean[32], rms[32];
	for (x=0; x < 32; x++) 
	{
		mean[x] = hist[x]->GetMean();
		rms[x] = hist[x]->GetRMS();
		//	for (int i=-16384;i<16384;i++) hist[x]->Fill(i);
	}

	for (x=0; x < 32; x++) {
		TF1 *gaus = new TF1("gaus"+x,"gaus",-16384,16384);
		if (!no_gui)
			c1->cd(x+1);
		gaus->FixParameter(0,0.0);
		gaus->SetParameter(1,mean[x]);
		gaus->SetParameter(2,rms[x]);
		if (no_gui)
			hist[x]->Fit(gaus,"L0","",mean[x]-3*rms[x],mean[x]+3*rms[x]);
		else
			hist[x]->Fit(gaus,"L","",mean[x]-3*rms[x],mean[x]+3*rms[x]);
		//hist[x]->Fit(gaus,"L","",histMin[x]-10,histMax[x]+10);
		hist[x]->GetXaxis()->SetRangeUser(histMin[x]-10,histMax[x]+10);
		//hist[x]->GetXaxis()->SetRangeUser(mean[x]-3*rms[x],mean[x]+3*rms[x]);
		printf("mean %f, RMS %f, fitted mean %f, fitted RMS %f\n",mean[x],rms[x],gaus->GetParameter(1),gaus->GetParameter(2));
		if (!no_gui)
			hist[x]->Draw();
		//meanVal[x]  = hist[x]->GetFunction("gaus")->GetParameter(1);
		//meanVal[x]  = hist[x]->GetMean() / 16383.0;
		plotX[x] = x;
		meanVal[x]  = gaus->GetParameter(1);
	}

	if (!no_gui)
	{
		c2 = new TCanvas("c2","c2");
		c2->cd();
		plot = new TGraph(32,plotX,meanVal);
		plot->Draw("a*");
	}

	double avg = 0;
	for (x=22; x < 32; x++) {
		avg += meanVal[x];
	}
	avg/=10;

	double adjVal[32];
	for (x=0; x < 32; x++) {
		adjVal[x] = (meanVal[x]-avg)/(meanVal[0]-avg);
		cout << "Idx=" << dec << x 
			<< " value=" << dec << meanVal[x] 
			<< " hex=0x" << hex << (uint)meanVal[x] 
			<< " adj=" << adjVal[x] << endl;
	}

	sum = 1;
	cout << "coeff: 1";
	for (x=1; x < 10; x++) {
		sum += adjVal[x];
		cout << "," << adjVal[x];
	}
	cout << endl;
	cout << "Sum=" << sum << endl;

	if (!no_gui)
		theApp.Run();
	return(0);

}

