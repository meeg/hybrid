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
#include <DataReadEvio.h>
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
	bool flip_channels = false;
	bool move_fitstart = false;
	bool read_temp = false;
	bool evio_format = false;
	int use_fpga = -1;
	int use_hybrid = -1;
	int num_events = -1;
	double fit_shift;
	ifstream calfile;
	TString inname = "";
	TString outdir = "";
	int cal_grp = -1;
	int cal_delay = 0;
	double delay_step = SAMPLE_INTERVAL/8;
	TCanvas         *c1;
	int allCounts[7][3][640][48];
	double allMeans[7][3][640][48];
	double allVariances[7][3][640][48];
	for (int fpga = 0;fpga<7;fpga++)
		for (int hyb = 0;hyb<3;hyb++)
			for (int chan=0;chan<640;chan++) {
				for (int i=0;i<48;i++) {
					allCounts[fpga][hyb][chan][i] = 0;
					allMeans[fpga][hyb][chan][i] = 0;
					allVariances[fpga][hyb][chan][i] = 0;
				}
			}

	DataRead        *dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	int            eventCount;
	int runCount;
	char            name[100];
	char            name2[100];
	char title[200];
	double chanChan[640];
	for (int i=0;i<640;i++) chanChan[i] = i;

	while ((c = getopt(argc,argv,"hfrg:o:s:ntH:F:e:E")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-f: plot Tp fits for each channel\n");
				printf("-g: force use of specified cal group\n");
				printf("-r: plot fit results\n");
				printf("-o: use specified output filename\n");
				printf("-n: don't use physical channel numbering\n");
				printf("-s: start fit at given delay after a first guess at T0\n");
				printf("-t: print temperature\n");
				printf("-F: use only specified FPGA\n");
				printf("-H: use only specified hybrid\n");
				printf("-e: stop after specified number of events\n");
				printf("-E: use EVIO file format\n");
				return(0);
				break;
			case 'f':
				plot_tp_fits = true;
				break;
			case 'r':
				plot_fit_results = true;
				break;
			case 'n':
				flip_channels = true;
				break;
			case 'g':
				force_cal_grp = true;
				cal_grp = atoi(optarg);
				break;
			case 'o':
				inname = optarg;
				outdir = optarg;
				if (outdir.Contains('/')) {
					outdir.Remove(outdir.Last('/')+1);
				}
				else outdir="";
				break;
			case 's':
				move_fitstart = true;
				fit_shift = atof(optarg);
				break;
			case 't':
				read_temp = true;
				break;
			case 'F':
				use_fpga = atoi(optarg);
				break;
			case 'H':
				use_hybrid = atoi(optarg);
				break;
			case 'e':
				num_events = atoi(optarg);
				break;
			case 'E':
				evio_format = true;
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
	gStyle->SetPalette(1,0);
	gStyle->SetOptStat("emrou");
	gStyle->SetStatW(0.2);                
	gStyle->SetStatH(0.1);                
	gStyle->SetTitleOffset(1.4,"y");
	gStyle->SetPadLeftMargin(0.15);
	c1 = new TCanvas("c1","c1",1200,900);

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

	ofstream tpfile;
	cout << "Writing Tp calibration to " << inname+".tp" << endl;
	tpfile.open(inname+".tp");
	tpfile << "#" << inname << endl;

	ofstream shapefile;
	cout << "Writing pulse shape to " << inname+".shape" << endl;
	shapefile.open(inname+".shape");
	shapefile << "#" << inname << endl;

	ofstream noisefile;
	cout << "Writing pulse noise to " << inname+".noise" << endl;
	noisefile.open(inname+".noise");
	noisefile << "#" << inname << endl;

	while (optind<argc)
	{
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

		runCount = atoi(dataRead->getConfig("RunCount").c_str());

		if (!force_cal_grp)
		{
			cal_grp = atoi(dataRead->getConfig("cntrlFpga:hybrid:apv25:CalGroup").c_str());
			cout<<"Read calibration group "<<cal_grp<<" from data file"<<endl;
		}

		cal_delay = atoi(dataRead->getConfig("cntrlFpga:hybrid:apv25:Csel").substr(4,1).c_str());
		cout<<"Read calibration delay "<<cal_delay<<" from data file"<<endl;
		if (cal_delay==0)
		{
			cal_delay=8;
			cout<<"Force cal_delay=8 to keep sample time in range"<<endl;
		}


		// Process each event
		eventCount = 0;
		do {
			int fpga = event.fpgaAddress();
			if (use_fpga!=-1 && fpga!=use_fpga) continue;
			if (eventCount%1000==0) printf("Event %d\n",eventCount);
			if (read_temp && !event.isTiFrame()) for (uint i=0;i<4;i++)
				if (event.temperature(i)!=0.0)
				{
					printf("Event %d, temperature #%d: %f\n",eventCount,i,event.temperature(i));
					read_temp = false;
				}
			for (x=0; x < event.count(); x++) {
				// Get sample
				sample  = event.sample(x);
				int hybrid = sample->hybrid();
				if (use_hybrid!=-1 && hybrid!=use_hybrid) continue;

				int channel = sample->channel();
				if (!flip_channels)
					channel += (4-sample->apv())*128;
				else
					channel += sample->apv()*128;

				if ( channel >= (5 * 128) ) {
					cout << "Channel " << dec << channel << " out of range" << endl;
					cout << "Apv = " << dec << sample->apv() << endl;
					cout << "Chan = " << dec << sample->channel() << endl;
				}

				if (((int)sample->channel()-cal_grp)%8!=0) continue;

				// Filter APVs
				if ( eventCount >= 20 ) {

					int sum = 0;
					for ( y=0; y < 6; y++ ) {
						sum += (int)sample->value(y);
					}
					sum-=6*sample->value(0);
					if (sum<0) continue;
					//int sgn = eventCount%2;
					for ( y=0; y < 6; y++ ) {
						int bin = 8*y+8-cal_delay;
						allCounts[fpga][hybrid][channel][bin]++;
						int value = sample->value(y);
						double delta = value-allMeans[fpga][hybrid][channel][bin];
						if (allCounts[fpga][hybrid][channel][bin]==1)
						{
							allMeans[fpga][hybrid][channel][bin] = value;
						}
						else
						{
							allMeans[fpga][hybrid][channel][bin] += delta/allCounts[fpga][hybrid][channel][bin];
						}
						allVariances[fpga][hybrid][channel][bin] += delta*(value-allMeans[fpga][hybrid][channel][bin]);
					}
				}
			}
			eventCount++;

		} while ( dataRead->next(&event));
		dataRead->close();
		if (eventCount != runCount)
		{
			printf("ERROR: events read = %d, runCount = %d\n",eventCount, runCount);
		}
		optind++;
	}


	TF1 *shapingFunction = new TF1("Shaping Function","[3]+[0]*(max(x-[1],0)/[2])*exp(1-((x-[1])/[2]))",-1.0*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL);
	for (int fpga = 0;fpga<7;fpga++)
		for (int hyb = 0;hyb<3;hyb++)
		{
			double chanNoise[640];
			double chanTp[640];
			double chanT0[640];
			double chanA[640];
			double chanChisq[640];
			for (int i=0;i<640;i++)
			{
				chanNoise[i] = 0;
				chanTp[i] = 0;
				chanT0[i] = 0;
				chanA[i] = 0;
				chanChisq[i] = 0;
				double yi[48], ey[48], ti[48];
				int ni = 0;
				TGraphErrors *fitcurve;

				double A, T0, Tp, A0, fit_start;

				for (int bin=0;bin<48;bin++)
				{
					if (allCounts[fpga][hyb][i][bin])
					{
						allVariances[fpga][hyb][i][bin]/=allCounts[fpga][hyb][i][bin];
						allVariances[fpga][hyb][i][bin]=sqrt(allVariances[fpga][hyb][i][bin]);
						yi[ni] = allMeans[fpga][hyb][i][bin];
						ey[ni] = allVariances[fpga][hyb][i][bin]/sqrt(allCounts[fpga][hyb][i][bin]);
						ti[ni] = (bin-8)*delay_step;
						ni++;
						chanNoise[i]+=allVariances[fpga][hyb][i][bin];
					}
				}
				if (ni==0) continue;
				chanNoise[i]/=ni;
				noisefile << fpga << "\t" << hyb << "\t" << i << "\t";
				noisefile << chanNoise[i] << endl;

				fitcurve = new TGraphErrors(ni,ti,yi,NULL,ey);
				shapingFunction->SetParameter(0,TMath::MaxElement(ni,yi)-yi[0]);
				shapingFunction->SetParameter(1,12.0);
				shapingFunction->SetParameter(2,50.0);
				shapingFunction->FixParameter(3,yi[0]);
				if (fitcurve->Fit(shapingFunction,"Q0","",-1*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL)==0)
				{
					A = shapingFunction->GetParameter(0);
					T0 = shapingFunction->GetParameter(1);
					Tp = shapingFunction->GetParameter(2);
					if (move_fitstart)
					{
						fit_start = T0+fit_shift;
						fitcurve->Fit(shapingFunction,"Q0","",fit_start,5*SAMPLE_INTERVAL);
						A = shapingFunction->GetParameter(0);
						T0 = shapingFunction->GetParameter(1);
						Tp = shapingFunction->GetParameter(2);
					}
					chanA[i] = A;
					chanT0[i] = T0;
					chanTp[i] = Tp;
					chanChisq[i] = shapingFunction->GetChisquare();
				} else
				{
					printf("Could not fit pulse shape for FPGA %d, hybrid %d, channel %d\n",fpga,hyb,i);
				}
				if (plot_tp_fits)
				{
					c1->Clear();
					fitcurve->Draw();
					if (move_fitstart)
					{
						shapingFunction->SetLineStyle(1);
						shapingFunction->SetLineWidth(1);
						shapingFunction->SetLineColor(2);
						shapingFunction->SetRange(fit_start,5*SAMPLE_INTERVAL);
						shapingFunction->DrawCopy("LSAME");
						shapingFunction->SetRange(-1*SAMPLE_INTERVAL,fit_start);
						shapingFunction->SetLineStyle(2);
						shapingFunction->Draw("LSAME");
					}
					else
					{
						shapingFunction->SetLineStyle(1);
						shapingFunction->SetLineWidth(1);
						shapingFunction->SetLineColor(2);
						shapingFunction->SetRange(-1*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL);
						shapingFunction->Draw("LSAME");
					}
					sprintf(name,"%s_tp_fit_F%d_H%d_%i.png",inname.Data(),fpga,hyb,i);
					c1->SaveAs(name);
				}
				delete fitcurve;

				shapefile << fpga << "\t" << hyb << "\t" << i << "\t";
				for (int j=0;j<ni;j++)
				{
					shapefile<<"\t"<<ti[j]-T0<<"\t"<<(yi[j]-A0)/A<<"\t"<<ey[j]/A;
				}
				shapefile<<endl;

				tpfile << fpga << "\t" << hyb << "\t" << i << "\t";
				tpfile << chanA[i]<<"\t"<<chanT0[i]<<"\t"<<chanTp[i]<<"\t"<<chanChisq[i]<<endl;
			}
			if (plot_fit_results)
			{
				c1->SetLogy(0);
				sprintf(name,"A_F%d_H%d",fpga,hyb);
				sprintf(name2,"%s_tp_F%d_H%d_A.png",inname.Data(),fpga,hyb);
				sprintf(title,"Fitted amplitude;Channel;Amplitude [ADC counts]");
				plotResults(title, name, name2, 640, chanChan, chanA, c1);

				c1->SetLogy(0);
				sprintf(name,"T0_F%d_H%d",fpga,hyb);
				sprintf(name2,"%s_tp_F%d_H%d_T0.png",inname.Data(),fpga,hyb);
				sprintf(title,"Fitted T0;Channel;T0 [ns]");
				plotResults(title, name, name2, 640, chanChan, chanT0, c1);

				c1->SetLogy(0);
				sprintf(name,"Tp_F%d_H%d",fpga,hyb);
				sprintf(name2,"%s_tp_F%d_H%d_Tp.png",inname.Data(),fpga,hyb);
				sprintf(title,"Fitted Tp;Channel;Tp [ns]");
				plotResults(title, name, name2, 640, chanChan, chanTp, c1);

				c1->SetLogy(1);
				sprintf(name,"Chisq_F%d_H%d",fpga,hyb);
				sprintf(name2,"%s_tp_F%d_H%d_Chisq.png",inname.Data(),fpga,hyb);
				sprintf(title,"Fit chisq;Channel;Chisq");
				plotResults(title, name, name2, 640, chanChan, chanChisq, c1);

				c1->SetLogy(0);
				sprintf(name,"Noise_F%d_H%d",fpga,hyb);
				sprintf(name2,"%s_tp_F%d_H%d_Noise.png",inname.Data(),fpga,hyb);
				sprintf(title,"Mean RMS noise per sample;Channel;Noise [ADC counts]");
				plotResults(title, name, name2, 640, chanChan, chanNoise, c1);
			}

		}

	// Close file
	tpfile.close();
	shapefile.close();
	noisefile.close();
	return(0);
}

