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
#include <TH2S.h>
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
	bool force_cal_grp = false;
	int cal_grp = -1;
	int cal_delay = 0;
	double delay_step = SAMPLE_INTERVAL/8;
	TCanvas         *c1;
	//TH2I            *histAll;
	TH2S            *histSamples[2][640];
	TH1D            *histSamples1D;
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
	int nChan[2] = {0};
	double grChan[2][640];
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
				force_cal_grp = true;
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
	if ( argc-optind==0) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}

	for (int channel=0;channel<640;channel++) {
		for (int sgn=0;sgn<2;sgn++)
		{
			sprintf(name,"samples_%s_%i",sgn?"neg":"pos",channel);
			histSamples[sgn][channel] = new TH2S(name,name,48,-0.5*delay_step,47.5*delay_step,16384,-0.5,16383.5);
		}
		histMin[channel] = 16384;
		histMax[channel] = 0;
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



	while (optind<argc)
	{
		// Attempt to open data file
		if ( ! dataRead.open(argv[optind]) ) return(2);

		dataRead.next(&event);
		dataRead.dumpConfig();
		dataRead.dumpStatus();

		if (!force_cal_grp)
		{
			cal_grp = atoi(dataRead.getConfig("cntrlFpga:hybrid:apv25:CalGroup").c_str());
			cout<<"Read calibration group "<<cal_grp<<" from data file"<<endl;
		}

		cal_delay = atoi(dataRead.getConfig("cntrlFpga:hybrid:apv25:Csel").substr(4,1).c_str());
		cout<<"Read calibration delay "<<cal_delay<<" from data file"<<endl;


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

				if ((channel-cal_grp)%8!=0) continue;
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
					}
					sum-=6*sample->value(0);
					int sgn = sum>0?0:1;
					for ( y=0; y < 6; y++ ) {
						histSamples[sgn][channel]->Fill((8*y+8-cal_delay)*delay_step,sample->value(y));
					}
					//tpfile<<"T0 " << fit_par[0] <<", A " << fit_par[1] << "Fit chisq " << chisq << ", DOF " << dof << ", prob " << TMath::Prob(chisq,dof) << endl;
				}
			}
			eventCount++;

		} while ( dataRead.next(&event) );
		dataRead.close();
		optind++;
	}

	double yi[48], ey[48], ti[48];
	int ni;
	TGraphErrors *fitcurve;
	TF1 *shapingFunction = new TF1("Shaping Function",
			"[3]+[0]*(max(x-[1],0)/[2])*exp(1-((x-[1])/[2]))",0, 6*SAMPLE_INTERVAL);
	c1 = new TCanvas("c1","c1",1200,900);

	for (int channel=0;channel<640;channel++) for (int sgn=0;sgn<2;sgn++) {
		if (plot_tp_fits) histSamples[sgn][channel]->GetYaxis()->SetRangeUser(histMin[channel],histMax[channel]);
		if (histSamples[sgn][channel]->GetEntries()>20)
		{
			if (plot_tp_fits)
			{
				c1->Clear();
				histSamples[sgn][channel]->Draw("colz");
			}
			//histSamples[sgn][n]->FitSlicesY();

			//sprintf(name,"samples_%s_%d_1",sgn?"neg":"pos",channel);
			//TH1F *means = (TH1F*)gDirectory->Get(name);
			//sprintf(name,"samples_%s_%d_2",sgn?"neg":"pos",channel);
			//TH1F *sigmas = (TH1F*)gDirectory->Get(name);
			ni=0;
			for (int i=0;i<48;i++)
			{
				sprintf(name,"samples_%s_%i_%i",sgn?"neg":"pos",channel,i);
				histSamples1D = histSamples[sgn][channel]->ProjectionY(name,i+1,i+1);

				if (histSamples1D->GetEntries()>20) 
				{
					if (histSamples1D->Fit("gaus","Q0")==0) {

						yi[ni]  = histSamples1D->GetFunction("gaus")->GetParameter(1);
						ey[ni]  = histSamples1D->GetFunction("gaus")->GetParError(1);
						ti[ni] = i*delay_step;
						ni++;
					}
					else {
						printf("Could not fit channel %d\n",channel);
					}
				}

				delete histSamples1D;
				//yi[i] = histSamples1D[sgn][n][i]->GetMean();
				//ey[i] = histSamples1D[sgn][n][i]->GetRMS();
				//yi[i] = means->GetBinContent(i+1);
				//ey[i] = sigmas->GetBinContent(i+1);
			}
			shapefile[sgn]<<channel<<"\t"<<ni;
			for (int i=0;i<ni;i++)
			{
				shapefile[sgn]<<"\t"<<ti[i]<<"\t"<<yi[i]<<"\t"<<ey[i];
			}
			shapefile[sgn]<<endl;
			fitcurve = new TGraphErrors(ni,ti,yi,NULL,ey);
			if (plot_tp_fits) fitcurve->Draw();
			shapingFunction->SetParameter(0,TMath::MaxElement(ni,yi)-yi[0]);
			shapingFunction->SetParameter(1,12.0);
			shapingFunction->SetParameter(2,50.0);
			shapingFunction->FixParameter(3,yi[0]);
			if (ni>0 && fitcurve->Fit(shapingFunction,plot_tp_fits?"Q":"Q0","",0,6*SAMPLE_INTERVAL)==0)
			{
				grChan[sgn][nChan[sgn]]=channel;
				grA[sgn][nChan[sgn]]=shapingFunction->GetParameter(0);
				if (sgn==1) grA[sgn][nChan[sgn]]*=-1;
				grT0[sgn][nChan[sgn]]=shapingFunction->GetParameter(1);
				grTp[sgn][nChan[sgn]]=shapingFunction->GetParameter(2);
				grChisq[sgn][nChan[sgn]]=shapingFunction->GetChisquare();
				nChan[sgn]++;
			}
			else
			{
				cout<<"Bad fit for pulses, channel "<<channel<<endl;
			}
			if (plot_tp_fits)
			{
				sprintf(name,"%s_tp_%s_%i.png",inname.Data(),sgn?"neg":"pos",channel);
				c1->SaveAs(name);
			}
			delete fitcurve;
		}
	}
	for (int sgn=0;sgn<2;sgn++)
	{
		for (int i=0;i<nChan[sgn];i++)
		{
			tpfile[sgn] <<grChan[sgn][i]<<"\t"<<grA[sgn][i]<<"\t"<<grT0[sgn][i]<<"\t"<<grTp[sgn][i]<<"\t"<<grChisq[sgn][i]<<endl;
		}
	}

	TGraph * graph;
	if (plot_fit_results) for (int sgn=0;sgn<2;sgn++)
	{
		c1->Clear();
		graph = new TGraph(nChan[sgn],grChan[sgn],grA[sgn]);
		sprintf(name,"A_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_A_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(nChan[sgn],grChan[sgn],grT0[sgn]);
		sprintf(name,"T0_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_T0_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(nChan[sgn],grChan[sgn],grTp[sgn]);
		sprintf(name,"Tp_%s",sgn?"neg":"pos");
		graph->SetTitle(name);
		graph->GetXaxis()->SetRangeUser(0,640);
		graph->Draw("a*");
		sprintf(name,"%s_tp_Tp_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		c1->Clear();
		graph = new TGraph(nChan[sgn],grChan[sgn],grChisq[sgn]);
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

