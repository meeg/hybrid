#include "meeg_utils.hh"
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

void doStats(int n, int nmin, int nmax, int *y, int &count, double &center, double &spread)
{
	double meansq = 0;
	double mean = 0;
	center = 0;
	count = 0;
	for (int i=nmin;i<=nmax;i++)
		if (y[i])
		{
			count += y[i];
			mean += i*y[i];
			meansq += pow(i,2)*y[i];
		}
	if (count==0)
	{
		center = 0;
		spread = 0;
		return;
	}
	mean /= count;
	meansq /= count;
	spread = sqrt(meansq-mean*mean);
	//center = mean;
	int medianCount=0;
	for (int i=nmin;i<=nmax;i++)
		if (y[i])
		{
			medianCount += 2*y[i];
			if (medianCount>count) 
			{
				center = i;
				break;
			}
			else if (medianCount==count)
			{
				center = i;
				i++;
				while (y[i]==0) i++;
				center += i;
				center /= 2.0;
				break;
			}
		}
}

void doStats(int n, int nmin, int nmax, short int *y, int &count, double &center, double &spread)
{
	int *newy = new int[n];
	for (int i=nmin; i<=nmax; i++) newy[i] = y[i];
	doStats(n, nmin, nmax, newy, count, center, spread);
}

void plotResults(char *name, char *filename, int n, double *x, double *y, TCanvas *canvas)
{
	if (n==0) return;
	int grpN[8]={0};
	double grpX[8][640];
	double grpY[8][640];
	TMultiGraph *mg = new TMultiGraph();
	TGraph *graph[8];
	int k;
	char nameN[100];
	for (int i = 0; i<n;i++)
	{
		k=((int)floor(x[i]+0.5))%8;
		grpX[k][grpN[k]]=x[i];
		grpY[k][grpN[k]]=y[i];
		grpN[k]++;
	}
	canvas->Clear();
	for (int i=0;i<8;i++) if (grpN[i]>0)
	{
		graph[i] = new TGraph(grpN[i],grpX[i],grpY[i]);
		sprintf(nameN,"%s_%d",name,i);
		graph[i]->SetTitle(nameN);
		graph[i]->SetName(nameN);
		graph[i]->SetMarkerColor(i+1);
		mg->Add(graph[i]);
	}
	mg->Draw("a*");
	canvas->SaveAs(filename);
	for (int i=0;i<8;i++) if (grpN[i]>0)
	{
		delete graph[i];
	}
	delete mg;
}

void plotResults2(char *name, char *name2, char *filename, int n, double *x, double *y, double *y2, TCanvas *canvas)
{
	if (n==0) return;
	TMultiGraph *mg = new TMultiGraph();
	int k;
	char nameN[100];

	int grpN[8]={0};
	double grpX[8][640];
	double grpY[8][640];
	TGraph *graph[8];

	double grpY2[8][640];
	TGraph *graph2[8];

	for (int i = 0; i<n;i++)
	{
		k=((int)floor(x[i]+0.5))%8;
		grpX[k][grpN[k]]=x[i];
		grpY[k][grpN[k]]=y[i];
		grpY2[k][grpN[k]]=y2[i];
		grpN[k]++;
	}
	for (int i=0;i<8;i++) if (grpN[i]>0)
	{
		graph[i] = new TGraph(grpN[i],grpX[i],grpY[i]);
		sprintf(nameN,"%s_%d",name,i);
		graph[i]->SetTitle(nameN);
		graph[i]->SetName(nameN);
		graph[i]->SetMarkerStyle(3);
		graph[i]->SetMarkerColor(i+1);
		mg->Add(graph[i]);

		graph2[i] = new TGraph(grpN[i],grpX[i],grpY2[i]);
		sprintf(nameN,"%s_%d",name2,i);
		graph2[i]->SetTitle(nameN);
		graph2[i]->SetName(nameN);
		graph2[i]->SetMarkerSize(0.25);
		graph2[i]->SetMarkerStyle(20);
		graph2[i]->SetMarkerColor(i+1);
		mg->Add(graph2[i]);
	}

	canvas->Clear();
	mg->Draw("ap");
	canvas->SaveAs(filename);
	for (int i=0;i<8;i++) if (grpN[i]>0)
	{
		delete graph[i];
		delete graph2[i];
	}
	delete mg;
}
