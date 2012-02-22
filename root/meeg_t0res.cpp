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
#include "ShapingCurve.hh"
#include "SmoothShapingCurve.hh"
#include "Samples.hh"
#include "Fitter.hh"
#include "AnalyticFitter.hh"
#include "LinFitter.hh"
#include <TMath.h>
#include "meeg_utils.hh"
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
	bool print_fit_status = false;
	bool force_cal_grp = false;
	bool ignore_cal_grp = false;
	bool use_shape = false;
	bool shift_t0 = false;
	int single_channel = -1;
	TString inname = "";
	int cal_grp = -1;
	int cal_delay = 0;
	double delay_step = SAMPLE_INTERVAL/8;
	TCanvas         *c1;
	//TH2F            *histAll;
	double          histMin[640];
	double          histMax[640];
	//TGraph          *mean;
	//TGraph          *sigma;
	int nChan[2] = {0, 0};
	double          grChan[2][640];
	double grT0[2][640], grT0_sigma[2][640], grT0_err[2][640];
	double grA[2][640], grA_sigma[2][640], grA_err[2][640];
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
	int            channel;
	uint            eventCount;
	double          sum;
	char            filename[100];
	char            name[100];
	char            name2[100];
	ShapingCurve *myShape[2][640];
	Fitter *myFitter[2][640];

	TH1F *histT0[2][640];
	TH1F *histA[2][640];
	TH1F *histT0_err[2][640];
	TH1F *histA_err[2][640];
	TH2F *histChiProb[2];
	TH2F *histT0_2d[2];
	TH2F *histA_2d[2];
	TH2I *pulse2D[2];
	TH2I *T0_A[2];
	int maxA[2] = {0, 0};
	int minA[2] = {16384, 16384};
	double maxT0[2] = {0.0, 0.0};
	double minT0[2] = {0.0, 0.0};


	while ((c = getopt(argc,argv,"hfsg:o:auc:")) !=-1)
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
				return(0);
				break;
			case 'f':
				print_fit_status = true;
				break;
			case 'u':
				use_shape = true;
				break;
			case 'a':
				ignore_cal_grp = true;
				break;
			case 's':
				shift_t0 = true;
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
				break;
			case '?':
				printf("Invalid option or missing option argument; -h to list options\n");
				return(1);
			default:
				abort();
		}

	initChan();

	gStyle->SetOptStat("nemrou");
	gStyle->SetPalette(1,0);
	gROOT->SetStyle("Plain");
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
		calfile >> calMean[channel];
		calfile >> calSigma[channel];
	}
	calfile.close();

	optind++;
	for (int sgn=0;sgn<2;sgn++)
		if (use_shape)
		{
			int ni;
			double ti[300], yi[300], ey[300];
			double shape_t0, shape_y0, shape_e0;
			sprintf(name,"%s.shape_%s",argv[optind],sgn?"neg":"pos");
			cout << "Reading shape calibration from "<<name<<endl;
			calfile.open(name);
			while (!calfile.eof()) {
				calfile >> channel;
				calfile >> ni;
				calfile >> shape_t0;
				calfile >> shape_y0;
				calfile >> shape_e0;
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
				ti[ni+j] = 300.0;
				yi[ni+j] = 0.0;
				ey[ni+j] = 1.0;
				ni+=j+1;

				myShape[sgn][channel] = new SmoothShapingCurve(ni,ti,yi);
				myFitter[sgn][channel] = new LinFitter(myShape[sgn][channel],6,1,1.0);
				myFitter[sgn][channel]->setSigmaNoise(calSigma[channel]);


				if (single_channel!=-1 && channel==single_channel)
				{
					c1->Clear();
					TSpline3 *tempspline = new TSpline3("tempspline",ti,yi,ni,"",0.0,yi[ni-1]);
					tempspline->Draw();
					sprintf(name,"spline_%s.png",sgn?"neg":"pos");
					c1->SaveAs(name);
				}

			}
			calfile.close();
		}
		else
		{
			sprintf(name,"%s.tp_%s",argv[optind],sgn?"neg":"pos");
			cout << "Reading Tp calibration from "<<name<<endl;
			calfile.open(name);
			while (!calfile.eof()) {
				calfile >> channel;
				calfile >> calA[sgn][channel];
				calfile >> calT0[sgn][channel];
				calfile >> calTp[sgn][channel];
				calfile >> calChisq[sgn][channel];

				//myShape[sgn][channel] = new SmoothShapingCurve(calTp[sgn][channel]);
				//myFitter[sgn][channel] = new LinFitter(myShape[sgn][channel],6,1,calSigma[channel]);
				myShape[sgn][channel] = new ShapingCurve(calTp[sgn][channel]);
				myFitter[sgn][channel] = new AnalyticFitter(myShape[sgn][channel],6,1,calSigma[channel]);
			}
			calfile.close();
		}
	optind++;


	// 2d histogram
	for (int sgn=0;sgn<2;sgn++)
	{
		sprintf(name,"Pulse_shape_%s",sgn?"Neg":"Pos");
		pulse2D[sgn] = new TH2I(name,name,520,-1.5*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL,500,-0.1,1.2);
		sprintf(name,"A_vs_T0_%s",sgn?"Neg":"Pos");
		T0_A[sgn] = new TH2I(name,name,500,-4*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL,500,0.0,2000.0);
		//T0_A[sgn] = new TH2I(name,name,500,0,2*SAMPLE_INTERVAL,500,800.0,1400.0);
		if (force_cal_grp)
		{
			sprintf(name,"Chisq_Prob_%s",sgn?"Neg":"Pos");
			histChiProb[sgn] = new TH2F(name,name,80,0,640,100,0,1.0);
			sprintf(name,"T0_%s",sgn?"Neg":"Pos");
			histT0_2d[sgn] = new TH2F(name,name,80,0,640,1000,-1*SAMPLE_INTERVAL,6*SAMPLE_INTERVAL);
			sprintf(name,"A_%s",sgn?"Neg":"Pos");
			histA_2d[sgn] = new TH2F(name,name,80,0,640,1000,0,2000.0);
		}
		else
		{
			sprintf(name,"Chisq_Prob_%s",sgn?"Neg":"Pos");
			histChiProb[sgn] = new TH2F(name,name,640,0,640,100,0,1.0);
			sprintf(name,"T0_%s",sgn?"Neg":"Pos");
			histT0_2d[sgn] = new TH2F(name,name,640,0,640,1000,-1*SAMPLE_INTERVAL,6*SAMPLE_INTERVAL);
			sprintf(name,"A_%s",sgn?"Neg":"Pos");
			histA_2d[sgn] = new TH2F(name,name,640,0,640,1000,0,2000.0);
		}
	}

	for (int n=0;n<640;n++) {
		histMin[n] = 16384;
		histMax[n] = 0;
		for (int sgn=0;sgn<2;sgn++)
		{
			sprintf(name,"T0_%s_%i",sgn?"neg":"pos",n);
			histT0[sgn][n] = new TH1F(name,name,1000,0,3*SAMPLE_INTERVAL);
			sprintf(name,"T0err_%s_%i",sgn?"neg":"pos",n);
			histT0_err[sgn][n] = new TH1F(name,name,1000,0,10.0);
			sprintf(name,"A_%s_%i",sgn?"neg":"pos",n);
			histA[sgn][n] = new TH1F(name,name,1000,0,2000.0);
			sprintf(name,"Aerr_%s_%i",sgn?"neg":"pos",n);
			histA_err[sgn][n] = new TH1F(name,name,1000,0,100.0);
		}
	}


	if (inname=="")
	{
		inname=argv[optind];

		inname.ReplaceAll(".bin","");
		if (inname.Contains('/')) {
			inname.Remove(0,inname.Last('/')+1);
		}
	}



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
	cout << "Writing pulse shape to " << inname+".shape_pos" << endl;
	outshapefile[0].open(inname+".shape_pos");
	cout << "Writing pulse shape to " << inname+".shape_neg" << endl;
	outshapefile[1].open(inname+".shape_neg");

	double samples[6];
	Samples *mySamples = new Samples(6,SAMPLE_INTERVAL);
	double fit_par[2], fit_err[2], chisq, chiprob;
	int dof;


	while (optind<argc)
	{
		// Attempt to open data file
		if ( ! dataRead.open(argv[optind]) ) return(2);

		dataRead.next(&event);
		dataRead.dumpConfig();
		//dataRead.dumpStatus();
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

		do {
			if (eventCount%1000==0) printf("Event %d\n",eventCount);
			for (x=0; x < event.count(); x++) {

				// Get sample
				sample  = event.sample(x);
				channel = (sample->apv() * 128) + sample->channel();
				//				if (sample->apv()==0 || sample->apv()==4) continue;

				if (single_channel!=-1 && channel !=single_channel) continue;
				if ( channel >= (5 * 128) ) {
					cout << "Channel " << dec << channel << " out of range" << endl;
					cout << "Apv = " << dec << sample->apv() << endl;
					cout << "Chan = " << dec << sample->channel() << endl;
				}

				if (!ignore_cal_grp && cal_grp!=-1 && (channel-cal_grp)%8!=0) continue;
				int n = channel;
				// Filter APVs
				if ( eventCount > 20 ) {

					int samplesAbove = 0;
					int samplesBelow = 0;
					sum = 0;
					for ( y=0; y < 6; y++ ) {
						value = sample->value(y);

						//vhigh = (value << 1) & 0x2AAA;
						//vlow  = (value >> 1) & 0x1555;
						//value = vlow | vhigh;


						if ( value < histMin[n] ) histMin[n] = value;
						if ( value > histMax[n] ) histMax[n] = value;
						samples[y] = value;
						samples[y] -= calMean[channel];
						//samples[y] -= sample->value(0);
						sum+=samples[y];
						if (samples[y]>5*calSigma[channel]) samplesAbove++;
						if (samples[y]<-5*calSigma[channel]) samplesBelow++;
					}
					if (sum<0)
						for ( y=0; y < 6; y++ ) {
							samples[y]*=-1;
						}
					if (samplesAbove>2 || samplesBelow>2)
					{
						int sgn = sum>0?0:1;
						mySamples->readEvent(samples,0.0);
						myFitter[sgn][n]->readSamples(mySamples);
						myFitter[sgn][n]->doFit();
						myFitter[sgn][n]->getFitPar(fit_par);
						myFitter[sgn][n]->getFitErr(fit_err);
						chisq = myFitter[sgn][n]->getChisq(fit_par);
						dof = myFitter[sgn][n]->getDOF();
						histT0[sgn][n]->Fill(fit_par[0]);
						histT0_err[sgn][n]->Fill(fit_err[0]);
						histA[sgn][n]->Fill(fit_par[1]);
						histA_err[sgn][n]->Fill(fit_err[1]);
						T0_A[sgn]->Fill(fit_par[0],fit_par[1]);
						chiprob = TMath::Prob(chisq,dof);
						if (print_fit_status) fitfile<<"Channel "<<channel << ", T0 " << fit_par[0] <<", A " << fit_par[1] << ", Fit chisq " << chisq << ", DOF " << dof << ", prob " << chiprob << endl;
						if (fit_par[0]>maxT0[sgn]) maxT0[sgn] = fit_par[0];
						if (fit_par[0]<minT0[sgn]) minT0[sgn] = fit_par[0];
						if (fit_par[1]>maxA[sgn]) maxA[sgn] = fit_par[1];
						if (fit_par[1]<minA[sgn]) minA[sgn] = fit_par[1];
						histT0_2d[sgn]->Fill(channel,fit_par[0]);
						histA_2d[sgn]->Fill(channel,fit_par[1]);
						histChiProb[sgn]->Fill(channel,chiprob);
						for ( y=0; y < 6; y++ ) {
							pulse2D[sgn]->Fill(y*SAMPLE_INTERVAL-fit_par[0],samples[y]/fit_par[1]);
						}
					}
					else
					{
						//					fitfile << "Pulse below threshold on channel "<<channel<<endl;
					}
				}
			}
			eventCount++;

		} while ( dataRead.next(&event) );
		optind++;
	}

	for (int n=0;n<640;n++) for (int sgn=0;sgn<2;sgn++) if (histT0[sgn][n]->GetEntries()>0) {
		grChan[sgn][nChan[sgn]]=n;

		/*
		   histT0[sgn][n]->Fit("gaus","Q0");
		   grT0[sgn][nChan[sgn]] = histT0[sgn][n]->GetFunction("gaus")->GetParameter(1);
		   grT0_sigma[sgn][nChan[sgn]] = histT0[sgn][n]->GetFunction("gaus")->GetParameter(2);
		   histT0_err[sgn][n]->Fit("gaus","Q0");
		   grT0_err[sgn][nChan[sgn]] = histT0_err[sgn][n]->GetFunction("gaus")->GetParameter(1);
		   histA[sgn][n]->Fit("gaus","Q0");
		   grA[sgn][nChan[sgn]] = histA[sgn][n]->GetFunction("gaus")->GetParameter(1);
		   grA_sigma[sgn][nChan[sgn]] = histA[sgn][n]->GetFunction("gaus")->GetParameter(2);
		   histA_err[sgn][n]->Fit("gaus","Q0");
		   grA_err[sgn][nChan[sgn]] = histA_err[sgn][n]->GetFunction("gaus")->GetParameter(1);
		   */
		grT0[sgn][nChan[sgn]] = histT0[sgn][n]->GetMean();
		if (shift_t0) grT0[sgn][nChan[sgn]] -= cal_delay*delay_step + calT0[sgn][n];
		grT0_sigma[sgn][nChan[sgn]] = histT0[sgn][n]->GetRMS();
		grT0_err[sgn][nChan[sgn]] = histT0_err[sgn][n]->GetMean();
		grA[sgn][nChan[sgn]] = histA[sgn][n]->GetMean();
		grA_sigma[sgn][nChan[sgn]] = histA[sgn][n]->GetRMS();
		grA_err[sgn][nChan[sgn]] = histA_err[sgn][n]->GetMean();

		outfile[sgn] <<n<<"\t"<<grT0[sgn][nChan[sgn]]<<"\t\t"<<grT0_sigma[sgn][nChan[sgn]]<<"\t\t"<<grT0_err[sgn][nChan[sgn]]<<"\t\t";
		outfile[sgn]<<grA[sgn][nChan[sgn]]<<"\t\t"<<grA_sigma[sgn][nChan[sgn]]<<"\t\t"<<grA_err[sgn][nChan[sgn]]<<endl;     
		nChan[sgn]++;
	}

	for (int sgn=0;sgn<2;sgn++)
	{
		pulse2D[sgn]->Draw("colz");
		sprintf(name,"Pulse_profile_%s_1",sgn?"Neg":"Pos");
		TH2I *tempPulse = (TH2I*) pulse2D[sgn]->Clone("temp_pulse");
		int *tempArray = new int[tempPulse->GetNbinsY()];
		double *pulse_yi = new double[tempPulse->GetNbinsX()];
		double *pulse_ti = new double[tempPulse->GetNbinsX()];
		double *pulse_ey = new double[tempPulse->GetNbinsX()];
		int pulse_ni = 0;

		double center, spread;
		int count;
		tempPulse->Rebin2D(10,5);
		for (int i=0;i<tempPulse->GetNbinsX();i++)
		{
			for (int j=0;j<tempPulse->GetNbinsY();j++) tempArray[j] = tempPulse->GetBinContent(i+1,j+1);

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
			outshapefile[sgn] << channel << "\t";
			outshapefile[sgn] << pulse_ni << "\t";
			for (int i=0; i<pulse_ni; i++)
			{
				outshapefile[sgn] << pulse_ti[i] << "\t";
				outshapefile[sgn] << pulse_yi[i] << "\t";
				outshapefile[sgn] << pulse_ey[i] << "\t";
			}
			outshapefile[sgn] << endl;
		}
		outshapefile[sgn].close();
		TGraphErrors *graphErrors = new TGraphErrors(pulse_ni,pulse_ti,pulse_yi,0,pulse_ey);
		graphErrors->Draw();
		sprintf(name,"%s_t0_pulse_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);
		delete pulse_yi;
		delete pulse_ti;
		delete pulse_ey;
		delete tempPulse;
		delete tempArray;
		delete graphErrors;
		/*
		   pulseProfile[sgn] = pulse2D[sgn]->ProfileX(name);
		   pulseProfile[sgn]->Rebin(10);
		   pulseProfile[sgn]->SetLineWidth(2.0);
		   pulseProfile[sgn]->Draw("SAME");
		   sprintf(name,"%s_t0_pulse_%s.png",inname.Data(),sgn?"neg":"pos");
		   c1->SaveAs(name);
		   for (channel=0;channel<640;channel++)
		   {
		   outshapefile[sgn] << channel << "\t";
		   outshapefile[sgn] << pulseProfile[sgn]->GetNbinsX() << "\t";
		   for (int i=0; i<pulseProfile[sgn]->GetNbinsX(); i++)
		   {
		   outshapefile[sgn] << pulseProfile[sgn]->GetBinCenter(i+1) << "\t";
		   outshapefile[sgn] << pulseProfile[sgn]->GetBinContent(i+1) << "\t";
		   outshapefile[sgn] << pulseProfile[sgn]->GetBinError(i+1) << "\t";
		   }
		   outshapefile[sgn] << endl;
		   }
		   outshapefile[sgn].close();
		   */

		T0_A[sgn]->GetYaxis()->SetRangeUser(minA[sgn],maxA[sgn]);
		T0_A[sgn]->GetXaxis()->SetRangeUser(minT0[sgn],maxT0[sgn]);
		T0_A[sgn]->Draw("colz");
		sprintf(name,"%s_t0_T0_A_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		histChiProb[sgn]->Draw("colz");
		sprintf(name,"%s_t0_chiprob_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		histT0_2d[sgn]->Draw("colz");
		sprintf(name,"%s_t0_T0_hist_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		histA_2d[sgn]->Draw("colz");
		sprintf(name,"%s_t0_A_hist_%s.png",inname.Data(),sgn?"neg":"pos");
		c1->SaveAs(name);

		sprintf(name,"T0_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_t0_T0_%s.png",inname.Data(),sgn?"neg":"pos");
		plotResults(name, name2, nChan[sgn], grChan[sgn], grT0[sgn], c1);

		sprintf(name,"T0_err_%s",sgn?"neg":"pos");
		sprintf(name2,"T0_spread_%s",sgn?"neg":"pos");
		sprintf(filename,"%s_t0_T0_sigma_%s.png",inname.Data(),sgn?"neg":"pos");
		plotResults2(name, name2, filename, nChan[sgn], grChan[sgn], grT0_err[sgn], grT0_sigma[sgn], c1);

		sprintf(name,"A_%s",sgn?"neg":"pos");
		sprintf(name2,"%s_t0_A_%s.png",inname.Data(),sgn?"neg":"pos");
		plotResults(name, name2, nChan[sgn], grChan[sgn], grA[sgn], c1);

		sprintf(name,"A_err_%s",sgn?"neg":"pos");
		sprintf(name2,"A_spread_%s",sgn?"neg":"pos");
		sprintf(filename,"%s_t0_A_sigma_%s.png",inname.Data(),sgn?"neg":"pos");
		plotResults2(name, name2, filename, nChan[sgn], grChan[sgn], grA_err[sgn], grA_sigma[sgn], c1);
	}

	// Start X-Windows
	//	theApp.Run();

	// Close file
	dataRead.close();
	if (print_fit_status) fitfile.close();
	outfile[0].close();
	outfile[1].close();
	return(0);
}

