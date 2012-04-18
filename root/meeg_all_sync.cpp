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
	bool read_temp = false;
	bool evio_format = false;
	bool no_gui = false;
	bool print_data = false;
	int use_fpga = -1;
	int use_hybrid = -1;
	int use_apv = -1;
	int num_events = -1;
	int c;


	DataRead        *dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	int            eventCount;
	int runCount;
	TString inname;
	TString outdir;
	char title[200];
	char name[200];
	

	TH1S            *hist[7][3][5][32];
	double          histMin[7][3][5][32];
	double          histMax[7][3][5][32];
	double          meanVal[32];
	double          plotX[32];
	ifstream        inFile;
	string          inLine;
	stringstream    inLineStream;
	string          inValue;
	stringstream    inValueStream;
	TGraph          *plot;

	while ((c = getopt(argc,argv,"ho:nmctH:F:A:e:Egd")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-o: use specified output filename\n");
				printf("-n: physical channel numbering\n");
				printf("-m: number channels in raw mux order\n");
				printf("-c: don't compute correlations\n");
				printf("-t: print temperature\n");
				printf("-F: use only specified FPGA\n");
				printf("-H: use only specified hybrid\n");
				printf("-A: use only specified APV\n");
				printf("-e: stop after specified number of events\n");
				printf("-E: use EVIO file format\n");
				printf("-g: no GUI\n");
				printf("-d: no printing of fits\n");
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
			case 'g':
				no_gui = true;
				break;
			case 'd':
				print_data = true;
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
			case 'A':
				use_apv = atoi(optarg);
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

	TCanvas         *c1;
	gROOT->SetStyle("Plain");
	gStyle->SetOptStat("emrou");
	gStyle->SetPalette(1,0);
	gStyle->SetStatW(0.2);                
	gStyle->SetStatH(0.1);                
	gStyle->SetTitleOffset(1.4,"y");
	gStyle->SetPadLeftMargin(0.15);

	// Start X11 view
	//   TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind != 1 ) {
		cout << "Usage: meeg_baseline data_file\n";
		return(1);
	}

	for (int fpga = 0;fpga<7;fpga++)
		for (int hyb = 0;hyb<3;hyb++)
			for (int apv = 0;apv<5;apv++)
				for (x=0; x < 32; x++) {
					sprintf(title,"sample_%d_F%d_H%d_A%d",x,fpga,hyb,apv);
					hist[fpga][hyb][apv][x] = new TH1S(title,title,16384,-0.5,16383.5);
					histMin[fpga][hyb][apv][x] = 16384;
					histMax[fpga][hyb][apv][x] = 0;
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
	//if (print_data)
	cout << "Writing calibration to " << inname+".filter" << endl;
	outfile.open(inname+".filter");
	outfile << inname << endl;

	//if (print_data)
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
	//if (print_data)
	cout << "Writing configuration to " <<outdir<<confname << endl;
	outconfig.open(outdir+confname);

	dataRead->next(&event);
	dataRead->dumpConfig(outconfig);
	outconfig.close();
	//dataRead.dumpStatus();

	runCount = atoi(dataRead->getConfig("RunCount").c_str());
	/*
	   double *moving_yi = new double[runCount];
	   double *moving_yi2 = new double[runCount];
	   */


	// Process each event
	eventCount = 0;

	do {
		int fpga = event.fpgaAddress();
		//printf("fpga %d\n",event.fpgaAddress());
		if (event.fpgaAddress()==7) 
		{
			//printf("not a data event\n");
			continue;
		}
		if (use_fpga!=-1 && fpga!=use_fpga) continue;
		//cout<<"  fpga #"<<event.fpgaAddress()<<"; number of samples = "<<event.count()<<endl;
		//if (print_data)
		if (eventCount%1000==0) printf("Event %d\n",eventCount);
		if (num_events!=-1 && eventCount >= num_events) break;
		if (read_temp && !event.isTiFrame()) for (uint i=0;i<4;i++)
			if (event.temperature(i)!=0.0)
			{
				printf("Event %d, temperature #%d: %f\n",eventCount,i,event.temperature(i));
				read_temp = false;
			}
		for ( y=0; y < 6; y++ ) {
			int idx = 10000;
			for (x=0; x < event.count(); x++) {
				// Get sample
				sample  = event.sample(x);
				int hyb = sample->hybrid();
				int apv = sample->apv();
				if (use_hybrid!=-1 && sample->hybrid()!=use_hybrid) continue;
				if (use_apv!=-1 && sample->apv()!=use_apv) continue;
				//printf("hybrid %d\n",sample->hybrid());


				int adcValue = sample->value(y) & 0x3FFF;
				int adcValid = sample->value(y) >> 15;
				//printf("fpga: %d, x: %d, val: %x, adcValid: %d\n",event.fpgaAddress(),x, adcValue, adcValid);
				if (adcValid)
				{
					//printf("fpga: %d, x: %d, val: %x, adcValid: %d\n",event.fpgaAddress(),x, adcValue, adcValid);

					//if ( idx==10000 && adcValue > 0x2000 ) idx = 0;
					if ( adcValue > 0x2000 ) {
						idx = 0;
						//printf("sync! event: %d, fpga: %d, hybrid: %d, apv: %d, samples: %d, x: %d, val: %x, adcValid: %d\n",eventCount,event.fpgaAddress(),sample->hybrid(),sample->apv(),y, x, adcValue, adcValid);
					}

					if ( idx < 32 ) {
						//cout << "Fill idx=" << dec << idx << " value=0x" << hex << value << endl;
						hist[fpga][hyb][apv][idx]->Fill(adcValue);
						if ( adcValue < histMin[fpga][hyb][apv][idx] ) histMin[fpga][hyb][apv][idx] = adcValue;
						if ( adcValue > histMax[fpga][hyb][apv][idx] ) histMax[fpga][hyb][apv][idx] = adcValue;
					}
					idx++;
				}
			}
		}
		eventCount++;

	} while ( dataRead->next(&event));
	dataRead->close();

	if (eventCount != runCount)
	{
		if (print_data)
			printf("ERROR: events read = %d, runCount = %d\n",eventCount, runCount);
	}


	if (!no_gui) {
		c1 = new TCanvas("c1","c1",1200,900);
		c1->Divide(8,4,0.0125,0.0125);
	}

	for (int fpga = 0;fpga<7;fpga++)
		for (int hyb = 0;hyb<3;hyb++)
			for (int apv = 0;apv<5;apv++)
			{
				if (hist[fpga][hyb][apv][0]->GetEntries()<10) continue;
				double mean[32], rms[32];
				for (x=0; x < 32; x++) 
				{
					mean[x] = hist[fpga][hyb][apv][x]->GetMean();
					rms[x] = hist[fpga][hyb][apv][x]->GetRMS();
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
						hist[fpga][hyb][apv][x]->Fit(gaus,"QL0","",mean[x]-3*rms[x],mean[x]+3*rms[x]);
					else
						hist[fpga][hyb][apv][x]->Fit(gaus,"QL","",mean[x]-3*rms[x],mean[x]+3*rms[x]);
					hist[fpga][hyb][apv][x]->GetXaxis()->SetRangeUser(histMin[fpga][hyb][apv][x]-100,histMax[fpga][hyb][apv][x]+100);
					//hist[x]->GetXaxis()->SetRangeUser(mean[x]-3*rms[x],mean[x]+3*rms[x]);
					if (print_data)
						printf("mean %f, RMS %f, fitted mean %f, fitted RMS %f\n",mean[x],rms[x],gaus->GetParameter(1),gaus->GetParameter(2));
					if (!no_gui)
						hist[fpga][hyb][apv][x]->Draw();
					//meanVal[x]  = hist[x]->GetFunction("gaus")->GetParameter(1);
					//meanVal[x]  = hist[x]->GetMean() / 16383.0;
					plotX[x] = x;
					meanVal[x]  = gaus->GetParameter(1);
				}

				if (!no_gui)
				{
					sprintf(name,"%s_fits_F%d_H%d_A%d.png",inname.Data(),fpga,hyb,apv);
					c1->SaveAs(name);
					delete c1;
					c1 = new TCanvas("c2","c2");
					c1->cd();
					plot = new TGraph(32,plotX,meanVal);
					plot->Draw("a*");
					sprintf(name,"%s_coeffs_F%d_H%d_A%d.png",inname.Data(),fpga,hyb,apv);
					c1->SaveAs(name);
				}

				double avg = 0;
				for (x=22; x < 32; x++) {
					avg += meanVal[x];
				}
				avg/=10;

				double adjVal[32];
				for (x=0; x < 32; x++) {
					adjVal[x] = (meanVal[x]-avg)/(meanVal[0]-avg);
					if (print_data)
						printf("Idx=%d value=%f hex=0x%x adj=%f\n",x,meanVal[x],(uint)meanVal[x],adjVal[x]);
				}

				double          sum = 0;
				for (x=0; x < 10; x++) {
					sum += adjVal[x];
				}
				//if (print_data)
				cout << "Sum=" << sum << endl;

				outfile << fpga << "\t" << hyb << "\t" << apv;
				for (x=0; x < 10; x++) {
					outfile << "\t" << adjVal[x];
				}
				outfile << endl;
			}

	// Close file
	outfile.close();
	delete dataRead;
	return(0);
}

