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
#include <TROOT.h>
#include <TCanvas.h>
#include <TMultiGraph.h>
#include <TApplication.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TStyle.h>
#include <stdarg.h>
#include <TrackerEvent.h>
#include <TrackerSample.h>
#include <Data.h>
#include <DataRead.h>
#include <DataReadEvio.h>
#include "ShapingCurve.hh"
#include "SmoothShapingCurve.hh"
#include "Samples.hh"
#include "Fitter.hh"
#include "AnalyticFitter.hh"
#include "LinFitter.hh"
#include <TMath.h>
#include "meeg_utils.hh"
#include <unistd.h>
#include <TMVA/TSpline1.h>
using namespace std;

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
	int c;
	bool subtract_T0 = false;
	bool print_fit_status = false;
	bool force_cal_grp = false;
	bool ignore_cal_grp = false;
	bool flip_channels = false;
	bool use_shape = false;
	bool shift_t0 = false;
	bool use_dist = false;
	bool evio_format = false;
	int fpga = -1;
	int hybrid = -1;
	int num_events = -1;
	double dist_window;
	bool make_shape = false;
	int single_channel = -1;
	TString inname = "";
	TString outdir = "";
	int cal_grp = -1;
	int cal_delay = 0;
	double delay_step = SAMPLE_INTERVAL/8;
	TCanvas         *c1;
	//TH2F            *histAll;
	double          histMin[640];
	double          histMax[640];
	//TGraph          *mean;
	//TGraph          *sigma;
	int nChan = 0;
	//double          grChan[640];
	//double grT0[640], grT0_sigma[640], grT0_err[640];
	//double grA[640], grA_sigma[640], grA_err[640];
	double          calMean[640][7] = {{0.0}};
	double          calSigma[640][7] = {{1.0}};
	double calTp[640] = {0.0};
	double calA[640] = {0.0};
	double calT0[640] = {0.0};
	double calChisq[640] = {0.0};
	DataRead        *dataRead;
	TrackerEvent    event;
	TrackerSample   *sample;
	uint            x;
	uint            y;
	uint            value;
	int            channel;
	int            eventCount;
	int runCount;
	double          sum;
	char            filename[100];
	char            name[100];
	char            name2[100];
	char title[200];
	ShapingCurve *myShape[640];
	Fitter *myFitter[640];

	TH1F *histT0[640];
	TH1F *histA[640];
	TH1F *histA_all;
	TH2F *histT0_2d;
	TH2F *histA_2d;
	TH2I *T0_A;
	/*
	TH1F *histT0_err[640];
	TH1F *histA_err[640];
	TH2F *histChiProb;
	TH2I *pulse2D;
	*/
	double maxA = 0.0;
	double minA = 16384.0;
	double maxT0 = 0.0;
	double minT0 = 200.0;
	TGraph *T0_dist;
	double T0_dist_b,T0_dist_m;


	while ((c = getopt(argc,argv,"hfsg:o:auc:d:tbnH:F:e:E")) !=-1)
		switch (c)
		{
			case 'h':
				printf("-h: print this help\n");
				printf("-f: print fit status to .fits\n");
				printf("-g: force use of specified cal group\n");
				printf("-c: use only specified channel\n");
				printf("-a: use all cal groups\n");
				printf("-o: use specified output filename\n");
				printf("-s: shift T0 by value from Tp cal file\n");
				printf("-u: use .shape cal instead of .tp\n");
				printf("-b: subtract value of T0 in .tp cal\n");
				printf("-t: make new .shape cal based on T0-A distribution\n");
				printf("-d: read and use .dist file, starting T0 window at specified value\n");
				printf("-n: physical channel numbering\n");
				printf("-F: use only specified FPGA\n");
				printf("-H: use only specified hybrid\n");
				printf("-e: stop after specified number of events\n");
				printf("-E: use EVIO file format\n");
				return(0);
				break;
			case 'f':
				print_fit_status = true;
				break;
			case 'u':
				use_shape = true;
				break;
			case 'n':
				flip_channels = true;
				break;
			case 'a':
				ignore_cal_grp = true;
				break;
			case 's':
				shift_t0 = true;
				break;
			case 'b':
				subtract_T0 = true;
				break;
			case 'c':
				single_channel = atoi(optarg);
				break;
			case 'g':
				cal_grp = atoi(optarg);
				force_cal_grp=true;
				break;
			case 'o':
				inname = optarg;
				outdir = optarg;
				if (outdir.Contains('/')) {
					outdir.Remove(outdir.Last('/')+1);
				}
				else outdir="";
				break;
			case 'd':
				dist_window = atof(optarg);
				use_dist=true;
				break;
			case 't':
				make_shape = true;
				break;
			case 'F':
				fpga = atoi(optarg);
				break;
			case 'H':
				hybrid = atoi(optarg);
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
	gStyle->SetOptStat("emrou");
	gStyle->SetPalette(1,0);
	gStyle->SetStatW(0.2);                
	gStyle->SetStatH(0.1);                
	gStyle->SetTitleOffset(1.4,"y");
	gStyle->SetPadLeftMargin(0.15);
	c1 = new TCanvas("c1","c1",1200,900);
	//c1->SetLogz();
	// Start X11 view
	//TApplication theApp("App",NULL,NULL);

	// Root file is the first and only arg
	if ( argc-optind<3 ) {
		cout << "Usage: meeg_t0res baseline_cal tp_cal data_file\n";
		return(1);
	}

	ifstream calfile;
	cout << "Reading baseline calibration from " << argv[optind] << endl;
	calfile.open(argv[optind]);
	while (!calfile.eof()) {
		calfile >> channel;
		if (calfile.eof()) break;
		for (int i=0;i<7;i++)
		{
			calfile >> calMean[channel][i];
			calfile >> calSigma[channel][i];
		}
	}
	calfile.close();

	optind++;

	//for (int sgn=0;sgn<2;sgn++)
	//{
		sprintf(name,"%s.tp_pos",argv[optind]);
		cout << "Reading Tp calibration from "<<name<<endl;
		calfile.open(name);
		while (!calfile.eof()) {
			calfile >> channel;
			if (calfile.eof()) break;
			calfile >> calA[channel];
			calfile >> calT0[channel];
			calfile >> calTp[channel];
			calfile >> calChisq[channel];

		}
		calfile.close();
	//}

	//for (int sgn=0;sgn<2;sgn++)
	//{
		if (use_shape)
		{
			int ni;
			double ti[300], yi[300], ey[300];
			double shape_t0, shape_y0, shape_e0;
			sprintf(name,"%s.shape_pos",argv[optind]);
			cout << "Reading shape calibration from "<<name<<endl;
			calfile.open(name);
			while (!calfile.eof()) {
				calfile >> channel;
				calfile >> ni;
				calfile >> shape_t0;
				calfile >> shape_y0;
				calfile >> shape_e0;
				if (calfile.eof()) break;
				int j=0;
				do
				{
					ti[j] = -100.0+5.0*j;
					yi[j] = 0.0;
					ey[j] = 1.0;
					j++;
				} while (ti[j]<shape_t0);
				ti[j] = shape_t0;
				yi[j] = shape_y0;
				ey[j] = shape_e0;
				for (int i=1;i<ni;i++)
				{
					calfile >> ti[i+j];
					calfile >> yi[i+j];
					calfile >> ey[i+j];
					//			if (ti[i]<0) yi[i]=0;
				}
				ni+=j;
				do
				{
					ti[ni] = ti[ni-1]+5.0;
					yi[ni] = yi[ni-1];
					ey[ni] = 1.0;
					ni++;
				} while (ti[ni-1]<300.0);

				myShape[channel] = new SmoothShapingCurve(ni,ti,yi);
				myFitter[channel] = new LinFitter(myShape[channel],6,1,TMath::Mean(6,calSigma[channel]));


				if (single_channel!=-1 && channel==single_channel)
					//if (channel==639)
				{
					c1->Clear();
					TSpline3 *tempspline = new TSpline3("tempspline",ti,yi,ni,"",0.0,yi[ni-1]);
					tempspline->Draw();
					tempspline->Print();
					sprintf(name,"spline_pos.png");
					c1->SaveAs(name);
				}
			}
			calfile.close();
		}
		else
		{
			for (channel = 0; channel<640; channel++)
			{
				//myShape[channel] = new SmoothShapingCurve(calTp[channel]);
				//myFitter[channel] = new LinFitter(myShape[channel],6,1,calSigma[channel]);
				myShape[channel] = new ShapingCurve(calTp[channel]);
				myFitter[channel] = new AnalyticFitter(myShape[channel],6,1,TMath::Mean(6,calSigma[channel]));
			}
		}

		if (use_dist)
		{
			int ni = 0;
			double ti[500], integral[500], amplitude[500];
			sprintf(name,"%s.dist_pos",argv[optind]);
			cout << "Reading T0-A calibration from "<<name<<endl;
			/*
			   ti[0] = -100.0;
			   integral[0] = -100.0;
			   amplitude[0] = 0.0;
			   ni++;
			   */
			calfile.open(name);
			while (!calfile.eof()) {
				calfile >> ti[ni];
				calfile >> amplitude[ni];
				calfile >> integral[ni];
				if (calfile.eof()) break;
				ni++;
			}
			calfile.close();
			/*
			   ti[ni] = ti[ni-1]+100.0;
			   integral[ni] = integral[ni-1]*2;
			   amplitude[ni] = 0.0;
			   ni++;
			   */
			//sprintf(name,"T0_dist_%s",sgn?"neg":"pos");
			//T0_dist = new TSpline3(name,ti,integral,ni);
			T0_dist = new TGraph(ni,ti,integral);
			T0_dist_m = SAMPLE_INTERVAL/(T0_dist->Eval(dist_window+SAMPLE_INTERVAL) - T0_dist->Eval(dist_window));
			T0_dist_b = dist_window - T0_dist->Eval(dist_window)*T0_dist_m;

			c1->Clear();
			T0_dist->Draw("al");
			sprintf(name,"T0_dist_pos.png");
			c1->SaveAs(name);
		}
	//}
	optind++;


	// 2d histogram
	{
		int sgn = 1;
		/*
		sprintf(name,"Pulse_shape_%s",sgn?"Neg":"Pos");
		sprintf(title,"Normalized pulse shape, %s pulses;Time [ns];Amplitude [normalized]",sgn?"negative":"positive");
		pulse2D = new TH2I(name,title,520,-1.5*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL,500,-0.1,1.2);
		*/
		sprintf(name,"A_vs_T0");
		sprintf(title,"Amplitude vs. T0;Time [ns];Amplitude [ADC counts]");
		T0_A = new TH2I(name,title,500,-4*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL,500,0.0,2000.0);
		//T0_A = new TH2I(name,name,500,0,2*SAMPLE_INTERVAL,500,800.0,1400.0);
		if (force_cal_grp)
		{
			//sprintf(name,"Chisq_Prob_%s",sgn?"Neg":"Pos");
			//sprintf(title,"Chisq probability of fit, %s pulses;Channel;Probability",sgn?"negative":"positive");
			//histChiProb = new TH2F(name,title,80,0,640,100,0,1.0);
			sprintf(name,"T0_%s",sgn?"Neg":"Pos");
			sprintf(title,"Fitted values of T0 relative to channel average, %s pulses;Channel;T0 [ns]",sgn?"negative":"positive");
			histT0_2d = new TH2F(name,title,80,0,640,1000,-1*SAMPLE_INTERVAL,6*SAMPLE_INTERVAL);
			sprintf(name,"A_%s",sgn?"Neg":"Pos");
			sprintf(title,"Fitted values of amplitude, %s pulses;Channel;Amplitude [ADC counts]",sgn?"negative":"positive");
			histA_2d = new TH2F(name,title,80,0,640,1000,0,2000.0);
		}
		else
		{
			//sprintf(name,"Chisq_Prob_%s",sgn?"Neg":"Pos");
			//sprintf(title,"Chisq probability of fit, %s pulses;Channel;Probability",sgn?"negative":"positive");
			//histChiProb = new TH2F(name,title,640,0,640,100,0,1.0);
			sprintf(name,"T0_%s",sgn?"Neg":"Pos");
			sprintf(title,"Fitted values of T0 relative to channel average, %s pulses;Channel;T0 [ns]",sgn?"negative":"positive");
			histT0_2d = new TH2F(name,title,640,0,640,1000,-1*SAMPLE_INTERVAL,6*SAMPLE_INTERVAL);
			sprintf(name,"A_%s",sgn?"Neg":"Pos");
			sprintf(title,"Fitted values of amplitude, %s pulses;Channel;Amplitude [ADC counts]",sgn?"negative":"positive");
			histA_2d = new TH2F(name,title,640,0,640,1000,0,2000.0);
		}
	}

	sprintf(name,"A");
	histA_all = new TH1F(name,name,1000,0,500.0);
	for (int n=0;n<640;n++) {
		histMin[n] = 16384;
		histMax[n] = 0;
			sprintf(name,"T0_%i",n);
			histT0[n] = new TH1F(name,name,1000,0,3*SAMPLE_INTERVAL);
			//sprintf(name,"T0err_%i",n);
			//histT0_err[n] = new TH1F(name,name,1000,0,10.0);
			sprintf(name,"A_%i",n);
			histA[n] = new TH1F(name,name,1000,0,2000.0);
			//sprintf(name,"Aerr_%i",n);
			//histA_err[n] = new TH1F(name,name,1000,0,100.0);
	}


	if (inname=="")
	{
		inname=argv[optind];

		inname.ReplaceAll(".bin","");
		if (inname.Contains('/')) {
			inname.Remove(0,inname.Last('/')+1);
		}
	}



	/*
	ofstream fitfile;
	if (print_fit_status) {
		cout << "Writing fit status to " << inname+".fits" << endl;
		fitfile.open(inname+".fits");
	}

	ofstream outfile[2];
	cout << "Writing T0 calibration to " << inname+".t0_pos" << endl;
	outfile[0].open(inname+".t0_pos");
	cout << "Writing T0 calibration to " << inname+".t0_neg" << endl;
	outfile[1].open(inname+".t0_neg");

	ofstream outshapefile[2];
	if (make_shape)
	{
		cout << "Writing pulse shape to " << inname+".shape_pos" << endl;
		outshapefile[0].open(inname+".shape_pos");
		cout << "Writing pulse shape to " << inname+".shape_neg" << endl;
		outshapefile[1].open(inname+".shape_neg");
	}

	ofstream outdistfile[2];
	cout << "Writing T0 and A distribution to " << inname+".dist_pos" << endl;
	outdistfile[0].open(inname+".dist_pos");
	cout << "Writing T0 and A distribution to " << inname+".dist_neg" << endl;
	outdistfile[1].open(inname+".dist_neg");
	*/

	double samples[6];
	Samples *mySamples = new Samples(6,SAMPLE_INTERVAL);
	double fit_par[2], fit_err[2], chisq, chiprob;
	int dof;


	while (optind<argc)
	{
		cout << "Reading data file " <<argv[optind] << endl;
		// Attempt to open data file
		if ( ! dataRead->open(argv[optind]) ) return(2);

		TString confname=argv[optind];
		confname.ReplaceAll(".bin",".conf");
		if (confname.Contains('/')) {
			confname.Remove(0,confname.Last('/')+1);
		}

		ofstream outconfig;
		cout << "Writing configuration to " <<outdir<<confname << endl;
		outconfig.open(outdir+confname);

		dataRead->next(&event);
		dataRead->dumpConfig(outconfig);
		outconfig.close();
		//dataRead.dumpStatus();
		
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
			if (fpga!=-1 && event.fpgaAddress()!=fpga) continue;
			if (eventCount%1000==0) printf("Event %d\n",eventCount);
			if (num_events > 0 && eventCount > num_events) break;
			for (x=0; x < event.count(); x++) {
				// Get sample
				sample  = event.sample(x);
				if (hybrid!=-1 && sample->hybrid()!=hybrid) continue;

				channel = sample->channel();
				if (flip_channels)
					channel += (4-sample->apv())*128;
				else
					channel += sample->apv()*128;
				//				if (sample->apv()==0 || sample->apv()==4) continue;

				if (single_channel!=-1 && channel !=single_channel) continue;
				if ( channel >= (5 * 128) ) {
					cout << "Channel " << dec << channel << " out of range" << endl;
					cout << "Apv = " << dec << sample->apv() << endl;
					cout << "Chan = " << dec << sample->channel() << endl;
				}

				//if (!ignore_cal_grp && cal_grp!=-1 && ((int)sample->channel()-cal_grp)%8!=0) continue;
				int n = channel;
				// Filter APVs
				if ( eventCount >= 20 ) {

					sum = 0;
					int nAboveThreshold = 0;
					for ( y=0; y < 6; y++ ) {
						value = sample->value(y);

						//vhigh = (value << 1) & 0x2AAA;
						//vlow  = (value >> 1) & 0x1555;
						//value = vlow | vhigh;


						if ( value < histMin[n] ) histMin[n] = value;
						if ( value > histMax[n] ) histMax[n] = value;
						samples[y] = value;
						samples[y] -= calMean[channel][y];
						//samples[y] -= sample->value(0);
						sum+=samples[y];
						if (samples[y]>2.0*TMath::Mean(6,calSigma[channel])) {
							nAboveThreshold++;
						}
					}
					//if ((samples[1]> samples[0] || samples[2]>samples[1]) && nAboveThreshold>2)
					if (nAboveThreshold>2)
					{
						//int sgn = sum>0?0:1;
						mySamples->readEvent(samples,0.0);
						myFitter[n]->readSamples(mySamples);
						myFitter[n]->doFit();
						myFitter[n]->getFitPar(fit_par);
						myFitter[n]->getFitErr(fit_err);
						chisq = myFitter[n]->getChisq(fit_par);
						dof = myFitter[n]->getDOF();
						//if (fit_par[1]!=fit_par[1]) 
						//{
						//	printf("%d\t%d\t%f\t%f meeg\n",channel,sgn,fit_par[0],fit_par[1]);
						//	mySamples->print();
						//	myFitter[n]->print_fit();
						//	myFitter[n]->printResiduals();
						//}

						if (use_dist)
						{
							if (fit_par[0]>dist_window && fit_par[0]<dist_window+SAMPLE_INTERVAL)
							{
								fit_par[0] = T0_dist_m*T0_dist->Eval(fit_par[0]) + T0_dist_b;
							}
							else continue;
						}

						if (subtract_T0) fit_par[0]-=calT0[channel];


						chiprob = TMath::Prob(chisq,dof);
						if (chiprob > 0.01) {
							histA_all->Fill(fit_par[1]);
							histT0[n]->Fill(fit_par[0]);
							histA[n]->Fill(fit_par[1]);
							histT0_2d->Fill(channel,fit_par[0]);
							histA_2d->Fill(channel,fit_par[1]);
							T0_A->Fill(fit_par[0],fit_par[1]);
						}
						/*
						   histT0_err[n]->Fill(fit_err[0]);
						   histA_err[n]->Fill(fit_err[1]);
						   if (print_fit_status) fitfile<<"Channel "<<channel << ", T0 " << fit_par[0] <<", A " << fit_par[1] << ", Fit chisq " << chisq << ", DOF " << dof << ", prob " << chiprob << endl;
						   if (fit_par[0]>maxT0) maxT0 = fit_par[0];
						   if (fit_par[0]<minT0) minT0 = fit_par[0];
						   if (fit_par[1]>maxA) maxA = fit_par[1];
						   if (fit_par[1]<minA) minA = fit_par[1];
						   histChiProb->Fill(channel,chiprob);
						   for ( y=0; y < 6; y++ ) {
						   pulse2D->Fill(y*SAMPLE_INTERVAL-fit_par[0],samples[y]/fit_par[1]);
						   }
						   */
					}
					else
					{
						//					fitfile << "Pulse below threshold on channel "<<channel<<endl;
					}
				}
			}
			eventCount++;

		} while ( dataRead->next(&event) );
		dataRead->close();
		if (eventCount != runCount)
		{
			printf("ERROR: events read = %d, runCount = %d\n",eventCount, runCount);
		}
		optind++;
	}

	/*
	   for (int n=0;n<640;n++) for (int sgn=0;sgn<2;sgn++) if (histT0[n]->GetEntries()>0) {
	   grChan[nChan]=n;

	   grT0[nChan] = histT0[n]->GetMean();
	   if (shift_t0) grT0[nChan] -= cal_delay*delay_step + calT0[n];
	   grT0_sigma[nChan] = histT0[n]->GetRMS();
	   grT0_err[nChan] = histT0_err[n]->GetMean();
	   grA[nChan] = histA[n]->GetMean();
	   grA_sigma[nChan] = histA[n]->GetRMS();
	   grA_err[nChan] = histA_err[n]->GetMean();

	   outfile <<n<<"\t"<<grT0[nChan]<<"\t\t"<<grT0_sigma[nChan]<<"\t\t"<<grT0_err[nChan]<<"\t\t";
	   outfile<<grA[nChan]<<"\t\t"<<grA_sigma[nChan]<<"\t\t"<<grA_err[nChan]<<endl;     
	   nChan++;
	   }
	   */

	{
		int sgn = 1;
		T0_A->GetYaxis()->SetRangeUser(minA,maxA);
		T0_A->GetXaxis()->SetRangeUser(minT0-5.0,maxT0+5.0);
		T0_A->Draw("colz");
		sprintf(name,"%s_t0_T0_A.png",inname.Data());
		c1->SaveAs(name);
		/*
		   pulse2D->Draw("colz");
		//sprintf(name,"Pulse_profile_%s_1",sgn?"Neg":"Pos");
		TH2I *tempPulse;
		int *tempArray;
		double *pulse_yi, *pulse_ti, *pulse_ey;
		TGraphErrors *graphErrors;
		if (make_shape)
		{
		tempPulse = (TH2I*) pulse2D->Clone("temp_pulse");
		tempArray = new int[tempPulse->GetNbinsY()];
		pulse_yi = new double[tempPulse->GetNbinsX()];
		pulse_ti = new double[tempPulse->GetNbinsX()];
		pulse_ey = new double[tempPulse->GetNbinsX()];
		int pulse_ni = 0;

		double center, spread;
		int count;
		tempPulse->Rebin2D(10,5);
		for (int i=0;i<tempPulse->GetNbinsX();i++)
		{
		for (int j=0;j<tempPulse->GetNbinsY();j++) tempArray[j] = (int) tempPulse->GetBinContent(i+1,j+1);

		doStats(tempPulse->GetNbinsY(),0,tempPulse->GetNbinsY()-1,tempArray,count,center,spread);
		if (count)
		{
		pulse_yi[pulse_ni] = tempPulse->GetYaxis()->GetBinCenter(1)+center*tempPulse->GetYaxis()->GetBinWidth(1);
		pulse_ey[pulse_ni] = spread*tempPulse->GetYaxis()->GetBinWidth(1)/sqrt(count);
		pulse_ti[pulse_ni] = tempPulse->GetXaxis()->GetBinCenter(1)+i*tempPulse->GetXaxis()->GetBinWidth(1);
		pulse_ni++;
		}
		}
		for (channel=0;channel<640;channel++)
		{
		outshapefile << channel << "\t";
		outshapefile << pulse_ni << "\t";
		for (int i=0; i<pulse_ni; i++)
		{
		outshapefile << pulse_ti[i] << "\t";
		outshapefile << pulse_yi[i] << "\t";
		outshapefile << pulse_ey[i] << "\t";
		}
		outshapefile << endl;
		}
		outshapefile.close();
		graphErrors = new TGraphErrors(pulse_ni,pulse_ti,pulse_yi,0,pulse_ey);
		graphErrors->Draw();
		}
		sprintf(name,"%s_t0_pulse_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);
		if (make_shape)
		{
		delete pulse_yi;
		delete pulse_ti;
		delete pulse_ey;
		delete tempPulse;
		delete tempArray;
		delete graphErrors;
		}


		tempArray = new int[T0_A->GetNbinsY()];
		int count_integral = 0;
		for (int i=0;i<T0_A->GetNbinsX();i++)
		{
		for (int j=0;j<T0_A->GetNbinsY();j++) tempArray[j] = (int) T0_A->GetBinContent(i+1,j+1);

		double center, spread;
		int count;
		doStats(T0_A->GetNbinsY(),0,T0_A->GetNbinsY()-1,tempArray,count,center,spread);
		count_integral+=count;
		if (count>0)
		{
			outdistfile << T0_A->GetXaxis()->GetBinCenter(1)+i*T0_A->GetXaxis()->GetBinWidth(1) << "\t";
			outdistfile << T0_A->GetYaxis()->GetBinCenter(1)+center*T0_A->GetYaxis()->GetBinWidth(1) << "\t";
			outdistfile << count_integral << "\t";
			outdistfile << endl;
		}
	}
	outdistfile.close();





	histChiProb->Draw("colz");
	sprintf(name,"%s_t0_chiprob_%s.png",inname.Data(),sgn?"neg":"pos");
	c1->SaveAs(name);
	*/

		histT0_2d->GetYaxis()->SetRangeUser(minT0-5.0,maxT0+5.0);
	histT0_2d->Draw("colz");
	sprintf(name,"%s_t0_T0_hist_%s.png",inname.Data(),sgn?"neg":"pos");
	c1->SaveAs(name);

	histA_2d->GetYaxis()->SetRangeUser(minA,maxA);
	histA_2d->Draw("colz");
	sprintf(name,"%s_t0_A_hist_%s.png",inname.Data(),sgn?"neg":"pos");
	c1->SaveAs(name);

	histA_all->Draw();
	sprintf(name,"%s_t0_A.png",inname.Data());
	c1->SaveAs(name);

	/*
	   sprintf(name,"T0_graph_%s",sgn?"neg":"pos");
	   sprintf(name2,"%s_t0_T0_%s.png",inname.Data(),sgn?"neg":"pos");
	   sprintf(title,"Mean fitted T0, %s pulses;Channel;T0 [ns]",sgn?"negative":"positive");
	   plotResults(title, name, name2, nChan, grChan, grT0, c1);

	   sprintf(name,"T0_err_%s",sgn?"neg":"pos");
	   sprintf(name2,"T0_spread_%s",sgn?"neg":"pos");
	   sprintf(filename,"%s_t0_T0_sigma_%s.png",inname.Data(),sgn?"neg":"pos");
	   sprintf(title,"Error and spread in fitted T0, %s pulses;Channel;T0 error [ns]",sgn?"negative":"positive");
	   plotResults2(title, name, name2, filename, nChan, grChan, grT0_err, grT0_sigma, c1);

	   sprintf(name,"A_graph_%s",sgn?"neg":"pos");
	   sprintf(name2,"%s_t0_A_%s.png",inname.Data(),sgn?"neg":"pos");
	   sprintf(title,"Mean fitted amplitude, %s pulses;Channel;Amplitude [ADC counts]",sgn?"negative":"positive");
	   plotResults(title, name, name2, nChan, grChan, grA, c1);

	   sprintf(name,"A_err_%s",sgn?"neg":"pos");
	   sprintf(name2,"A_spread_%s",sgn?"neg":"pos");
	   sprintf(filename,"%s_t0_A_sigma_%s.png",inname.Data(),sgn?"neg":"pos");
	   sprintf(title,"Error and spread in fitted amplitude, %s pulses;Channel;Amplitude error [ADC counts]",sgn?"negative":"positive");
	   plotResults2(title, name, name2, filename, nChan, grChan, grA_err, grA_sigma, c1);
	   */
	}

	// Start X-Windows
	//	theApp.Run();

	// Close file
	//if (print_fit_status) fitfile.close();
	//outfile[0].close();
	//outfile[1].close();
	return(0);
}

