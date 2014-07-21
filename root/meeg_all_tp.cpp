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
#include <DevboardEvent.h>
#include <DevboardSample.h>
#include <TriggerEvent.h>
#include <TriggerSample.h>
#include <Data.h>
#include <DataRead.h>
#include <DataReadEvio.h>
#include <TMath.h>
#include <TMultiGraph.h>
#include <TGraphErrors.h>
#include <unistd.h>
#include "meeg_utils.hh"

using namespace std;

#define MAX_RCE 4
#define MAX_FEB 4
#define MAX_HYB 4

// Process the data
// Pass root file to open as first and only arg.
int main ( int argc, char **argv ) {
    int c;
    bool plot_tp_fits = false;
    bool plot_fit_results = false;
    bool force_cal_grp = false;
    bool flip_channels = true;
    bool move_fitstart = false;
    bool read_temp = true;
    int hybrid_type = 0;
    bool evio_format = false;
    bool triggerevent_format = false;
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
    int hybridCount[MAX_RCE][MAX_FEB][MAX_HYB];
    int **allCounts[MAX_RCE][MAX_FEB][MAX_HYB];
    double **allMeans[MAX_RCE][MAX_FEB][MAX_HYB];
    double **allVariances[MAX_RCE][MAX_FEB][MAX_HYB];
    for (int rce = 0;rce<MAX_RCE;rce++)
        for (int fpga = 0;fpga<MAX_FEB;fpga++)
            for (int hyb = 0;hyb<MAX_HYB;hyb++) {
                hybridCount[rce][fpga][hyb] = 0;
            }

    DataRead        *dataRead;
    DevboardEvent    event;
    TriggerEvent    triggerevent;
    TriggerSample   *triggersample = new TriggerSample();
    int		samples[6];
    int            eventCount;
    int runCount;
    char            name[100];
    char            name2[100];
    char title[200];
    double chanChan[640];
    for (int i=0;i<640;i++) chanChan[i] = i;

    while ((c = getopt(argc,argv,"hfrg:o:s:nt:H:F:e:EV")) !=-1)
        switch (c)
        {
            case 'h':
                printf("-h: print this help\n");
                printf("-f: plot Tp fits for each channel\n");
                printf("-g: force use of specified cal group\n");
                printf("-r: plot fit results\n");
                printf("-o: use specified output filename\n");
                printf("-n: DAQ (Ryan's) channel numbering\n");
                printf("-s: start fit at given delay after a first guess at T0\n");
                printf("-t: hybrid type (1 for old test run hybrid, 2 for new 2014 hybrid)\n");
                printf("-F: use only specified FPGA\n");
                printf("-H: use only specified hybrid\n");
                printf("-e: stop after specified number of events\n");
                printf("-E: use EVIO file format\n");
                printf("-V: use TriggerEvent event format\n");
                return(0);
                break;
            case 'f':
                plot_tp_fits = true;
                break;
            case 'r':
                plot_fit_results = true;
                break;
            case 'n':
                flip_channels = false;
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
                hybrid_type = atoi(optarg);
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
            case 'V':
                triggerevent_format = true;
                break;
            case '?':
                printf("Invalid option or missing option argument; -h to list options\n");
                return(1);
            default:
                abort();
        }

    if (hybrid_type==0) {
        printf("WARNING: no hybrid type set; use -t to specify old or new hybrid\n");
        printf("Configured for old (test run) hybrid\n");
        hybrid_type = 1;
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

    if (!force_cal_grp) cal_grp = 0;
    cal_delay = 1;

    while (optind<argc)
    {
        cout << "Reading data file " <<argv[optind] << endl;
        // Attempt to open data file
        if ( ! dataRead->open(argv[optind]) ) {
            printf("bad file: %s\n",argv[optind]);
        }

        bool readOK;

        if (triggerevent_format) {
            dataRead->next(&triggerevent);
        } else {
            dataRead->next(&event);
        }

        if (!evio_format) {
            TString confname=argv[optind];
            confname.ReplaceAll(".bin","");
            confname.Append(".conf");
            if (confname.Contains('/')) {
                confname.Remove(0,confname.Last('/')+1);
            }

            ofstream outconfig;
            cout << "Writing configuration to " <<outdir<<confname << endl;
            outconfig.open(outdir+confname);
            outconfig << dataRead->getConfigXml();
            outconfig << endl;
            outconfig << dataRead->getStatusXml();
            outconfig.close();

            runCount = atoi(dataRead->getConfig("RunCount").c_str());

            if (!force_cal_grp)
            {
                string cgrp = dataRead->getConfig("cntrlFpga:hybrid:apv25:CalGroup");
                if (cgrp.length()==0) cgrp = dataRead->getConfig("FrontEndTestFpga:FebCore:Hybrid:apv25:CalGroup");
                cal_grp = atoi(cgrp.c_str());
                cout<<"Read calibration group "<<cal_grp<<" from data file"<<endl;
            }

            string csel = dataRead->getConfig("cntrlFpga:hybrid:apv25:Csel");
            if (csel.length()==0) csel = dataRead->getConfig("FrontEndTestFpga:FebCore:Hybrid:apv25:Csel");
            cal_delay = atoi(csel.substr(4,1).c_str());
            cout<<"Read calibration delay "<<cal_delay<<" from data file"<<endl;
            if (cal_delay==0)
            {
                cal_delay=8;
                cout<<"Force cal_delay=8 to keep sample time in range"<<endl;
            }
        }


        // Process each event
        eventCount = 0;
        do {
            int rce = 0;
            int fpga = 0;
            int samplecount;

            if (triggerevent_format) {
                samplecount = triggerevent.count();
            } else {
                fpga = event.fpgaAddress();
                samplecount = event.count();
                if (read_temp && !event.isTiFrame()) for (uint i=0;i<4;i++) {
                    printf("Event %d, temperature #%d: %f\n",eventCount,i,event.temperature(i,hybrid_type==1));
                    read_temp = false;
                }
            }
            if (!triggerevent_format && fpga==7) 
            {
                //printf("not a data event\n");
                continue;
            }
            if (eventCount%1000==0) printf("Event %d\n",eventCount);
            if (num_events!=-1 && eventCount >= num_events) break;
            for (int x=0; x < samplecount; x++) {
                int hyb;
                int apv;
                int apvch;
                int channel;

                bool goodSample = true;

                // Get sample
                if (triggerevent_format) {
                    triggerevent.sample(x,triggersample);
                    rce = triggersample->rceAddress();
                    fpga = triggersample->febAddress();
                    hyb = triggersample->hybrid();
                    apv = triggersample->apv();
                    apvch = triggersample->channel();
                    goodSample = (!triggersample->head() && !triggersample->tail());
                    for ( int y=0; y < 6; y++ ) {
                        //printf("%x\n",sample->value(y));
                        samples[y] = triggersample->value(y);
                    }
                } else {
                    DevboardSample *sample  = event.sample(x);
                    hyb = sample->hybrid();
                    apv = sample->apv();
                    apvch = sample->channel();
                    for ( int y=0; y < 6; y++ ) {
                        //printf("%x\n",sample->value(y));
                        samples[y] = sample->value(y) & 0x3FFF;
                        if (samples[y]==0) goodSample = false;
                    }
                }
                if (use_fpga!=-1 && fpga!=use_fpga) continue;
                if (use_hybrid!=-1 && hyb!=use_hybrid) continue;
                if (!goodSample) continue;

                channel = apvch;

                if (flip_channels)
                    channel += (4-apv)*128;
                else
                    channel += apv*128;

                if ( channel >= (5 * 128) ) {
                    cout << "Channel " << dec << channel << " out of range" << endl;
                    cout << "Apv = " << dec << apv << endl;
                    cout << "Chan = " << dec << apvch << endl;
                }

                if ((apvch-cal_grp)%8!=0) continue;

                // Filter APVs
                if ( eventCount >= 20 ) {
                    if (hybridCount[rce][fpga][hyb]==0) {
                        printf("found new hybrid: rce = %d, feb = %d, hyb = %d\n",rce,fpga,hyb);
                        //allCounts[rce][fpga][hyb] = new int[640][48];
                        //allMeans[rce][fpga][hyb] = new double[640][48];
                        //allVariances[rce][fpga][hyb] = new double[640][48];
                        allCounts[rce][fpga][hyb] = new int*[640];
                        allMeans[rce][fpga][hyb] = new double*[640];
                        allVariances[rce][fpga][hyb] = new double*[640];
                        for (int i=0;i<640;i++) {
                            allCounts[rce][fpga][hyb][i] = new int[48];
                            allMeans[rce][fpga][hyb][i] = new double[48];
                            allVariances[rce][fpga][hyb][i] = new double[48];
                            for (int j=0;j<48;j++) {
                                allCounts[rce][fpga][hyb][i][j] = 0;
                                allMeans[rce][fpga][hyb][i][j] = 0.0;
                                allVariances[rce][fpga][hyb][i][j] = 0.0;
                            }
                        }
                    }
                    hybridCount[rce][fpga][hyb]++;

                    int sum = 0;
                    for ( int y=0; y < 6; y++ ) {
                        sum += samples[y];
                    }
                    sum-=6*samples[0];
                    if (sum<0) continue;
                    //int sgn = eventCount%2;
                    for ( int y=0; y < 6; y++ ) {
                        int bin = 8*y+8-cal_delay;
                        allCounts[rce][fpga][hyb][channel][bin]++;
                        double delta = samples[y]-allMeans[rce][fpga][hyb][channel][bin];
                        if (allCounts[rce][fpga][hyb][channel][bin]==1)
                        {
                            allMeans[rce][fpga][hyb][channel][bin] = samples[y];
                        }
                        else
                        {
                            allMeans[rce][fpga][hyb][channel][bin] += delta/allCounts[rce][fpga][hyb][channel][bin];
                        }
                        allVariances[rce][fpga][hyb][channel][bin] += delta*(samples[y]-allMeans[rce][fpga][hyb][channel][bin]);
                    }
                }
            }
            eventCount++;

            if (triggerevent_format) {
                readOK = dataRead->next(&triggerevent);
            } else {
                readOK = dataRead->next(&event);
            }
        } while (readOK);
        dataRead->close();
        if (eventCount != runCount)
        {
            printf("ERROR: events read = %d, runCount = %d\n",eventCount, runCount);
        }
        optind++;
        if (evio_format) {
            if (!force_cal_grp) cal_grp++;
            if (cal_grp==8)
            {
                cal_grp = 0;
                cal_delay++;
            }
        }
    }


    TF1 *shapingFunction = new TF1("Shaping Function","[3]+[0]*(max(x-[1],0)/[2])*exp(1-((x-[1])/[2]))",-1.0*SAMPLE_INTERVAL,5*SAMPLE_INTERVAL);
    for (int rce = 0;rce<MAX_RCE;rce++)
        for (int fpga = 0;fpga<MAX_FEB;fpga++)
            for (int hyb = 0;hyb<MAX_HYB;hyb++)
                if (hybridCount[rce][fpga][hyb])
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
                            if (allCounts[rce][fpga][hyb][i][bin])
                            {
                                allVariances[rce][fpga][hyb][i][bin]/=allCounts[rce][fpga][hyb][i][bin];
                                allVariances[rce][fpga][hyb][i][bin]=sqrt(allVariances[rce][fpga][hyb][i][bin]);
                                yi[ni] = allMeans[rce][fpga][hyb][i][bin];
                                ey[ni] = allVariances[rce][fpga][hyb][i][bin]/sqrt(allCounts[rce][fpga][hyb][i][bin]);
                                ti[ni] = (bin-8)*delay_step;
                                ni++;
                                chanNoise[i]+=allVariances[rce][fpga][hyb][i][bin];
                            }
                        }
                        if (ni==0) continue;
                        chanNoise[i]/=ni;
                        noisefile << rce << "\t" << fpga << "\t" << hyb << "\t" << i << "\t";
                        noisefile << chanNoise[i] << endl;

                        fitcurve = new TGraphErrors(ni,ti,yi,NULL,ey);
                        shapingFunction->SetParameter(0,TMath::MaxElement(ni,yi)-yi[0]);
                        shapingFunction->SetParameter(1,12.0);
                        shapingFunction->SetParameter(2,50.0);
                        shapingFunction->FixParameter(3,yi[0]);
                        A0 = yi[0];
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
                            fitcurve->Draw("AL");
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

                        shapefile << rce << "\t" << fpga << "\t" << hyb << "\t" << i << "\t";
                        for (int j=0;j<ni;j++)
                        {
                            shapefile<<"\t"<<ti[j]-T0<<"\t"<<(yi[j]-A0)/A<<"\t"<<ey[j]/A;
                        }
                        shapefile<<endl;

                        tpfile << rce << "\t" << fpga << "\t" << hyb << "\t" << i << "\t";
                        tpfile << chanA[i]<<"\t"<<chanT0[i]<<"\t"<<chanTp[i]<<"\t"<<chanChisq[i]<<endl;
                    }
                    if (plot_fit_results)
                    {
                        c1->SetLogy(0);
                        sprintf(name,"A_R%d_F%d_H%d",rce,fpga,hyb);
                        sprintf(name2,"%s_tp_R%d_F%d_H%d_A.png",inname.Data(),rce,fpga,hyb);
                        sprintf(title,"Fitted amplitude;Channel;Amplitude [ADC counts]");
                        plotResults(title, name, name2, 640, chanChan, chanA, c1);

                        c1->SetLogy(0);
                        sprintf(name,"T0_R%d_F%d_H%d",rce,fpga,hyb);
                        sprintf(name2,"%s_tp_R%d_F%d_H%d_T0.png",inname.Data(),rce,fpga,hyb);
                        sprintf(title,"Fitted T0;Channel;T0 [ns]");
                        plotResults(title, name, name2, 640, chanChan, chanT0, c1);

                        c1->SetLogy(0);
                        sprintf(name,"Tp_R%d_F%d_H%d",rce,fpga,hyb);
                        sprintf(name2,"%s_tp_R%d_F%d_H%d_Tp.png",inname.Data(),rce,fpga,hyb);
                        sprintf(title,"Fitted Tp;Channel;Tp [ns]");
                        plotResults(title, name, name2, 640, chanChan, chanTp, c1);

                        c1->SetLogy(0);
                        sprintf(name,"Chisq_R%d_F%d_H%d",rce,fpga,hyb);
                        sprintf(name2,"%s_tp_R%d_F%d_H%d_Chisq.png",inname.Data(),rce,fpga,hyb);
                        sprintf(title,"Fit chisq;Channel;Chisq");
                        plotResults(title, name, name2, 640, chanChan, chanChisq, c1);

                        c1->SetLogy(0);
                        sprintf(name,"Noise_R%d_F%d_H%d",rce,fpga,hyb);
                        sprintf(name2,"%s_tp_R%d_F%d_H%d_Noise.png",inname.Data(),rce,fpga,hyb);
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

