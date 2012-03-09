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
#include <TMultiGraph.h>
#include <TGraphErrors.h>
#include <unistd.h>
#include "meeg_utils.hh"

using namespace std;

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
	int c;
	bool plot_tp_fits = false;
	bool plot_fit_results = false;
	bool force_cal_grp = false;
	bool use_baseline_cal = false;
	ifstream calfile;
	TString inname = "";
	int cal_grp = -1;
	int cal_delay = 0;
	double delay_step = SAMPLE_INTERVAL/8;
	TCanvas         *c1;
	//TH2I            *histAll;
	short *allSamples[2][640][48] = {{{NULL}}};
	//bool hasSamples[2][640][48] = {{{false}}};
	//TH1D            *histSamples1D;
	int          histMin[640];
	for (int i=0;i<640;i++) histMin[i]=16384;
	int          histMax[640];
	for (int i=0;i<640;i++) histMax[i]=0;
	//TGraph          *mean;
	//TGraph          *sigma;
	DataRead        dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	int            value;
	uint            channel;
	uint            eventCount;
	double          sum;
	char            name[100];
	char            name2[100];
	char title[200];
	int nChan[2] = {0};
	double grChan[2][640];
	double grTp[2][640];
	double grA[2][640];
	double grT0[2][640];
	double grChisq[2][640];
	double          calMean[640][7] = {{0.0}};
	double          calSigma[640][7] = {{1.0}};

	while ((c = getopt(argc,argv,"hfrg:o:b:")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-f: plot Tp fits for each channel\n");
				printf("-g: force use of specified cal group\n");
				printf("-r: plot fit results\n");
				printf("-o: use specified output filename\n");
				printf("-b: use specified baseline cal file\n");
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
			case 'o':
				inname = optarg;
				break;
			case 'b':
				use_baseline_cal = true;
				cout << "Reading baseline calibration from " << optarg << endl;
				calfile.open(optarg);
				while (!calfile.eof()) {
					calfile >> channel;
					for (int i=0;i<7;i++)
					{
						calfile >> calMean[channel][i];
						calfile >> calSigma[channel][i];
					}
				}
				calfile.close();
				break;
			case '?':
				printf("Invalid option or missing option argument; -h to list options\n");
				return(1);
			default:
				abort();
		}

	gROOT->SetStyle("Plain");
	gStyle->SetPalette(1,0);
	gStyle->SetOptStat("emrou");
	gStyle->SetStatW(0.2);                
	gStyle->SetStatH(0.1);                
	gStyle->SetTitleOffset(1.4,"y");
	gStyle->SetPadLeftMargin(0.15);
	c1 = new TCanvas("c1","c1",1200,900);

	// Start X11 view
	//TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind==0) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}


	if (inname == "")
	{
		inname=argv[optind];

		inname.ReplaceAll(".bin","");
		if (inname.Contains('/')) {
			inname.Remove(0,inname.Last('/')+1);
		}
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

	ofstream noisefile[2];
	cout << "Writing pulse noise to " << inname+".noise_pos" << endl;
	noisefile[0].open(inname+".noise_pos");
	cout << "Writing pulse noise to " << inname+".noise_neg" << endl;
	noisefile[1].open(inname+".noise_neg");


	while (optind<argc)
	{
		// Attempt to open data file
		if ( ! dataRead.open(argv[optind]) ) return(2);

		dataRead.next(&event);
		dataRead.dumpConfig();
		//dataRead.dumpStatus();
		for (uint i=0;i<4;i++)
			printf("Temperature #%d: %f\n",i,event.temperature(i));

		if (!force_cal_grp)
		{
			cal_grp = atoi(dataRead.getConfig("cntrlFpga:hybrid:apv25:CalGroup").c_str());
			cout<<"Read calibration group "<<cal_grp<<" from data file"<<endl;
		}

		cal_delay = atoi(dataRead.getConfig("cntrlFpga:hybrid:apv25:Csel").substr(4,1).c_str());
		cout<<"Read calibration delay "<<cal_delay<<" from data file"<<endl;
		if (cal_delay==0)
		{
			cal_delay=8;
			cout<<"Force cal_delay=8 to keep sample time in range"<<endl;
		}


		// Process each event
		eventCount = 0;
		//bool goodEvent;

		do {
			//goodEvent = true;
			//for (x=0; x < event.count(); x++) {
			//	sample  = event.sample(x);
			//	channel = (sample->apv() * 128) + sample->channel();
			//	if (channel==32 && sample->value(1)>8482 && sample->value(1)>7800) goodEvent = false; 
			//}
			//if (eventCount%2==0) printf("Event %d is %s\n",eventCount,goodEvent?"good":"bad");
			//goodEvent = true;
			if (eventCount%1000==0) printf("Event %d\n",eventCount);
			//if (goodEvent) 
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
						value = (int)sample->value(y);

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
					//int sgn = sum>0?0:1;
					int sgn = eventCount%2;
					for ( y=0; y < 6; y++ ) {
						if (allSamples[sgn][channel][8*y+8-cal_delay]==NULL)
						{
							allSamples[sgn][channel][8*y+8-cal_delay] = new short[16384];
							for (int i=0;i<16384;i++) allSamples[sgn][channel][8*y+8-cal_delay][i]=0;
						}
						allSamples[sgn][channel][8*y+8-cal_delay][sample->value(y)]++;
					}
					//tpfile<<"T0 " << fit_par[0] <<", A " << fit_par[1] << "Fit chisq " << chisq << ", DOF " << dof << ", prob " << TMath::Prob(chisq,dof) << endl;
				}
			}
			eventCount++;

		} while ( dataRead.next(&event));
		dataRead.close();
		optind++;
	}

	double yi[48], ey[48], ti[48];
	int ni;
	TGraphErrors *fitcurve;
	TF1 *shapingFunction = new TF1("Shaping Function",
			"[3]+[0]*(max(x-[1],0)/[2])*exp(1-((x-[1])/[2]))",-1.0*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL);

	double chanNoise[2][640]={{0.0}};
	double chanChan[640];
	//TH1S *histSamples1D = new TH1S("h1","h1",16384,-0.5,16383.5);
	TH2S *histSamples;
	double A, T0, Tp, A0;

	for (int i=0;i<640;i++) chanChan[i] = i;

	for (int channel=0;channel<640;channel++) for (int sgn=0;sgn<2;sgn++) {
		ni=0;
		for (int i=0;i<48;i++) if (allSamples[sgn][channel][i]!=NULL) 
		{
			int nsamples = 0;
			double rms;
			doStats(16384, histMin[channel], histMax[channel], allSamples[sgn][channel][i], nsamples, yi[ni], rms);
			if (use_baseline_cal) yi[ni] -= calMean[channel][i/8];
			//if (use_baseline_cal) yi[ni] += calMean[channel][i/8]-2*calMean[channel][6];

			ey[ni] = rms/sqrt((double)nsamples);
			chanNoise[sgn][channel]+=rms;
			//histSamples1D->Reset();
			//printf("TH!: %f, %f\n", histSamples1D->GetEntries(),(histSamples1D->GetRMS()*histSamples1D->GetRMS()-yi[ni]*yi[ni]));
			//for (int j=histMin[channel];j<=histMax[channel];j++)
			//{
			//histSamples1D->SetBinContent(j+1,allSamples[sgn][channel][i][j]);
			//}
			//if (histSamples1D->Fit("gaus","Q0")==0) {
			//yi[ni]  = histSamples1D->GetFunction("gaus")->GetParameter(1);
			//ey[ni]  = histSamples1D->GetFunction("gaus")->GetParError(1);
			//}
			//else {
			//	printf("Could not fit channel %d, polarity %d, sample %d\n",channel,sgn,i);
			//}

			ti[ni] = (i-8)*delay_step;
			ni++;
		}
		if (ni==0) continue;
		chanNoise[sgn][channel]/=ni;

		if (plot_tp_fits)
		{
			sprintf(name,"samples_%s_%i",sgn?"neg":"pos",channel);
			sprintf(title,"APV25 pulse shape, channel %d, %s pulses;Time [ns];Amplitude [ADC counts]",channel,sgn?"negative":"positive");
			if (use_baseline_cal)
				histSamples = new TH2S(name,title,48,-8.5*delay_step,39.5*delay_step,16384,-0.5-calMean[channel][6],16383.5-calMean[channel][6]);
			else 
				histSamples = new TH2S(name,title,48,-8.5*delay_step,39.5*delay_step,16384,-0.5,16383.5);
			c1->Clear();
			for (int i=0;i<48;i++) if (allSamples[sgn][channel][i]!=NULL) 
			{
				for (int j=histMin[channel];j<=histMax[channel];j++)
				{
					if (use_baseline_cal)
						histSamples->Fill((i-8)*delay_step,j-calMean[channel][i/8],allSamples[sgn][channel][i][j]);
					//histSamples->Fill((i-8)*delay_step,j+calMean[channel][i/8]-2*calMean[channel][6],allSamples[sgn][channel][i][j]);
					else
						histSamples->Fill((i-8)*delay_step,j,allSamples[sgn][channel][i][j]);
				}
			}
			if (use_baseline_cal)
				histSamples->GetYaxis()->SetRangeUser(histMin[channel]-calMean[channel][6],histMax[channel]-calMean[channel][6]);
			else
				histSamples->GetYaxis()->SetRangeUser(histMin[channel],histMax[channel]);
			histSamples->Draw("colz");
		}

		fitcurve = new TGraphErrors(ni,ti,yi,NULL,ey);
		if (plot_tp_fits) fitcurve->Draw();
		if (sgn==0) shapingFunction->SetParameter(0,TMath::MaxElement(ni,yi)-yi[0]);
		else shapingFunction->SetParameter(0,TMath::MinElement(ni,yi)-yi[0]);
		shapingFunction->SetParameter(1,12.0);
		shapingFunction->SetParameter(2,50.0);
		shapingFunction->FixParameter(3,yi[0]);
		if (ni>0)
		{
			if (fitcurve->Fit(shapingFunction,"Q0","",-1*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL)==0)
			{
				//if (ni>6)
				//	fitcurve->Fit(shapingFunction,"Q0","",grT0[sgn][nChan[sgn]]+20.0,5*SAMPLE_INTERVAL);
				grChan[sgn][nChan[sgn]]=channel;
				A = shapingFunction->GetParameter(0);
				T0 = shapingFunction->GetParameter(1);
				Tp = shapingFunction->GetParameter(2);
				A0 = shapingFunction->GetParameter(3);
				grA[sgn][nChan[sgn]]=A;
				if (sgn==1) grA[sgn][nChan[sgn]]*=-1;
				grT0[sgn][nChan[sgn]]=T0;
				grTp[sgn][nChan[sgn]]=Tp;
				grChisq[sgn][nChan[sgn]]=shapingFunction->GetChisquare();
				nChan[sgn]++;
			}
			else
			{
				printf("Could not fit pulse shape for channel %d, polarity %d\n",channel,sgn);
			}
		}
		if (plot_tp_fits)
		{
			shapingFunction->SetRange(-1*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL);
			shapingFunction->Draw("LSAME");
			sprintf(name,"%s_tp_fit_%s_%i.png",inname.Data(),sgn?"neg":"pos",channel);
			c1->SaveAs(name);
			delete histSamples;
		}
		delete fitcurve;

		shapefile[sgn]<<channel<<"\t"<<ni;
		for (int i=0;i<ni;i++)
		{
			shapefile[sgn]<<"\t"<<ti[i]-T0<<"\t"<<(yi[i]-A0)/A<<"\t"<<ey[i]/A;
		}
		shapefile[sgn]<<endl;
	}
	for (int sgn=0;sgn<2;sgn++)
	{
		for (int i=0;i<nChan[sgn];i++)
		{
			tpfile[sgn] <<grChan[sgn][i]<<"\t"<<grA[sgn][i]<<"\t"<<grT0[sgn][i]<<"\t"<<grTp[sgn][i]<<"\t"<<grChisq[sgn][i]<<endl;
		}
		for (int i=0;i<640;i++) noisefile[sgn]<<i<<"\t"<<chanNoise[sgn][i]<<endl;
	}

	if (plot_fit_results) for (int sgn=0;sgn<2;sgn++)
	{
		c1->SetLogy(0);
		sprintf(name,"A_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_tp_A_%s.png",inname.Data(),sgn?"neg":"pos");
		sprintf(title,"Fitted amplitude, %s pulses;Channel;Amplitude [ADC counts]",sgn?"negative":"positive");
		plotResults(title, name, name2, nChan[sgn], grChan[sgn], grA[sgn], c1);

		c1->SetLogy(0);
		sprintf(name,"T0_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_tp_T0_%s.png",inname.Data(),sgn?"neg":"pos");
		sprintf(title,"Fitted T0, %s pulses;Channel;T0 [ns]",sgn?"negative":"positive");
		plotResults(title, name, name2, nChan[sgn], grChan[sgn], grT0[sgn], c1);

		c1->SetLogy(0);
		sprintf(name,"Tp_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_tp_Tp_%s.png",inname.Data(),sgn?"neg":"pos");
		sprintf(title,"Fitted Tp, %s pulses;Channel;Tp [ns]",sgn?"negative":"positive");
		plotResults(title, name, name2, nChan[sgn], grChan[sgn], grTp[sgn], c1);

		c1->SetLogy(1);
		sprintf(name,"Chisq_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_tp_Chisq_%s.png",inname.Data(),sgn?"neg":"pos");
		sprintf(title,"Fit chisq, %s pulses;Channel;Chisq",sgn?"negative":"positive");
		plotResults(title, name, name2, nChan[sgn], grChan[sgn], grChisq[sgn], c1);

		c1->SetLogy(0);
		sprintf(name,"Noise_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_tp_Noise_%s.png",inname.Data(),sgn?"neg":"pos");
		sprintf(title,"Mean RMS noise per sample, %s pulses;Channel;Noise [ADC counts]",sgn?"negative":"positive");
		plotResults(title, name, name2, 640, chanChan, chanNoise[sgn], c1);

	}

	// Start X-Windows
	//theApp.Run();

	// Close file
	dataRead.close();
	tpfile[0].close();
	tpfile[1].close();
	shapefile[0].close();
	shapefile[1].close();
	noisefile[0].close();
	noisefile[1].close();
	return(0);
}

