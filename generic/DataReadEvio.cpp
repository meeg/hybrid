//-----------------------------------------------------------------------------
// File          : DataReadEvio.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Read data & configuration from disk
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------

#include <DataReadEvio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
using namespace std;

// Constructor
DataReadEvio::DataReadEvio ( ) {
	debug_=false;
	//  debug_=true;
	fd_ = -1;
	maxbuf=MAXEVIOBUF;
	fragment_offset[0] = 2;
	fragment_offset[1]=1;
	fragment_offset[2]=1;
	depth=0;
	nbanks=0;

	tee_fpga = 0;
}

// Deconstructor
DataReadEvio::~DataReadEvio ( ) { }


// Open file
bool DataReadEvio::open ( string file) {
	int status;
	char * filename = (char *) malloc((file.size()+1)*sizeof(char));
	strcpy(filename,file.c_str());
	if((status=evOpen(filename,"r",&fd_))!=0) {
		//      printf("\n ?Unable to open file %s, status=%d\n\n",file,status);
		cout<<"Unable to open file "<<file<<", status="<<status<<endl;
		return(false);
	}
	return(true);
}



/*
// Get next data record
Data *DataReadEvio::next ( ) {
Data *tmp = new Data;
if ( next(tmp) ) return(tmp);
else {
delete tmp;
return(NULL);
}
}
*/




// Get next data record
	bool DataReadEvio::next (Data *data) {
		if (tee.count() > tee_fpga)
		{
			Data *source_data = tee.getFPGAData(tee_fpga);
			data->copy(source_data->data(),source_data->size());
			tee_fpga++;
			if (tee.count()==tee_fpga)
			{
				tee.restart();
				tee_fpga = 0;
			}
			return true;
		}
		printf("reading an EVIO event\n");
		bool nodata = true;
		int nevents=0;
		int status;
		nbanks=0;
		if ( fd_ < 0 ) {
			cout<<"DataReadEvio::next error fd_<0...no file open"<<endl;
			return(false);
		}

		do{    
			unsigned int *buf = (unsigned int*)malloc(maxbuf*sizeof(unsigned int));
			status = evRead(fd_,buf,maxbuf);
			if(status==S_SUCCESS){
				nevents++;
				//  here, get the offset and the length of the SVT data in the buffer (buf)
				//  do this by stepping through the banks until get an SVT bank
				//  then hand make a "Data" object 
				eventInfo(buf);
				printf("evtTag = %d\n",evtTag);
				if(evtTag==1){
					inSVT=false;  // reset the inSVT flag
					parse_fragment(buf, BANK,&tee);
					nodata=false;  
				}else{
					if(evtTag==20)return(false); //this is the end of data
					//otherwise, just skip it. 
					cout<<"Not a data event...skipping"<<endl;
				}
			} else if (status==EOF)
			{
				cout << "end of file" << endl;
				return(false);
			}else{
				cout<<"oops...broke trying to evRead; error code "<<status<<endl;

				return(false);
			}
			delete buf;
		}  while(nodata);
		if(debug_)cout<<"Found a data event"<<endl;
		return(next(data));
	}


void DataReadEvio::eventInfo(unsigned int *buf) {

	evtTag         = (buf[1]>>16)&0xffff;
	evtType        = (buf[1]>>8)&0x3f;
	evtNum         = buf[1]&0xff;

}


void DataReadEvio::parse_fragment(unsigned int *buf, int fragment_type,TrackerEvioEvent* tee) {

	int length,type, padding=0;
	unsigned short tag;
	unsigned char num;

	//  cout<<"DataReadEvio::parse_fragment"<<endl;
	/* get type-dependent info */
	switch(fragment_type) {
		case BANK:
			length      = buf[0]+1;
			tag         = (buf[1]>>16)&0xffff;
			type        = (buf[1]>>8)&0x3f;
			padding     = (buf[1]>>14)&0x3;
			num         = buf[1]&0xff;
			break;

		case SEGMENT:
			length      = (buf[0]&0xffff)+1;
			type        = (buf[0]>>16)&0x3f;
			padding     = (buf[0]>>22)&0x3;
			tag         = (buf[0]>>24)&0xff;
			num         = -1;  /* doesn't have num */
			break;

		case TAGSEGMENT:
			length      = (buf[0]&0xffff)+1;
			type        = (buf[0]>>16)&0xf;
			tag         = (buf[0]>>20)&0xfff;
			num         = -1;   /* doesn't have num */
			break;

		default:
			printf("?illegal fragment_type in dump_fragment: %d",fragment_type);
			exit(EXIT_FAILURE);
			break;
	}

	/* fragment data */
	if(debug_)cout<<"Spitting out fragment header data: "<<endl;
	if(debug_)cout<<fragment_offset[fragment_type]<<"  "<<type<<"  "<<padding<<endl;
	if(debug_)cout<<tag<<"   "<<length<<"  "<<fragment_type<<endl;
	if(tag==2&&fragment_type==BANK){
		inSVT=true;
		if(debug_)cout<<"Here are the SVT Blocks!"<<endl;
	}

	makeTrackerBanks(&buf[fragment_offset[fragment_type]], type,
			length-fragment_offset[fragment_type], padding, tag,tee);


}

int DataReadEvio::getFragType(int type){
	switch (type) {
		case 0x1:
			cout<<"Found an unsigned int "<<endl;
			return(-1);
			break;
		case 0xe:
		case 0x10:
			cout<<"Found a bank"<<endl;
			return(BANK);
			break;
			/* segment */
		case 0xd:
		case 0x20:
			cout<<"Found a segment"<<endl;
			return(SEGMENT);
			break;

			/* tagsegment */
		case 0xc:
			cout<<"Found a tag segment"<<endl;
			return(TAGSEGMENT);
			break;
		default:
			printf("?illegal fragment_type in dump_fragment: %d",type);
			exit(EXIT_FAILURE);
			break;
	}
}



/**
 * This routine parses evio data and puts it into a "Data" object
 *
 * @param data     pointer to data 
 * @param type     type of evio data (ie. short, bank, etc)
 * @param length   length of data in 32 bit words
 * @param padding  number of bytes to be ignored at end of data
 */
void  DataReadEvio::makeTrackerBanks(unsigned int *data, int type, int length, int padding, int tag,TrackerEvioEvent* tee) {
	int p=0;
	int i,j;
	int fLen,fTag,dLen,dTag;
	char *c;
	switch (type) {
		/* unsigned 32 bit int */
		case 0x1:
			if(inSVT){
				nbanks++;
				if(debug_){
					cout<<"Dumping this data block to trackerBank! bank #"<<nbanks<<endl;
					for(i=0; i<length; i+=n32) {
						for(j=i; j<min((i+n32),length); j++) {
							cout<<j<<"   "<<data[j]<<endl;
						}
					}    
				}
				TrackerEvent* tb=new TrackerEvent();
				uint *data_  = (uint *)malloc((length+1) * sizeof(uint));
				memcpy(data_+1,data,length*sizeof(uint));
				data_[0] = tag;
				if (tag==7) data_[0]+=0x80000000;
				tb->copy(data_,length+1);
				if(debug_){
					cout<<"Made TrackerBank for FPGA = "<<tb->fpgaAddress()<<endl;    
				}
				tee->addFPGAData(tb);
				if(debug_)cout<<"Added it to EvioEvent"<<endl;
			}
			break;
			/* composite */
		case 0xf:
			fLen=data[0]&0xffff;
			fTag=(data[0]>>20)&0xfff;
			c=(char*)&data[1];

			dLen=(data[fLen+1])&0xffff;
			dTag=(data[fLen+1]>>20)&0xfff;
			for(i=0; i<dLen; i+=n32) {
				for(j=i; j<min((i+n32),dLen); j++) {
					//this is left here because we may want to do something with calorimeter block....
				}
			}
			break;
		case 0xe:
		case 0x10:
			if(debug_)cout<<"isSVTData:: found a bank"<<endl;
			while(p<length) {
				parse_fragment(&data[p],BANK,tee);
				p+=data[p]+1;
			}
			break;

			/* segment */
		case 0xd:
		case 0x20:
			while(p<length) {
				parse_fragment(&data[p],SEGMENT,tee);
				p+=(data[p]&0xffff)+1;
			}
			break;

			/* tagsegment */
		case 0xc:
			while(p<length) {
				parse_fragment(&data[p],TAGSEGMENT,tee);
				p+=(data[p]&0xffff)+1;
			}
			break;
		default:
			cout<<"Wha???"<<endl;
			break;

	}
}
