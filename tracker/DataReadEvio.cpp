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
	fragment_offset[0] = 2;//BANK
	fragment_offset[1]=1;//SEGMENT
	fragment_offset[2]=1;//TAGSEGMENT
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


void DataReadEvio::close () {
	evClose(fd_);
	fd_ = -1;
}

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
	if(debug_)printf("reading an EVIO event\n");
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
			if(debug_)printf("evtTag = %d\n",evtTag);
			if(evtTag==1){
				inSVT=false;  // reset the inSVT flag
				parse_event(buf);
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

void DataReadEvio::parse_event(unsigned int *buf) {
	int length,type, padding=0;
	unsigned short tag;
	unsigned short num;
	length      = buf[0]+1;
	tag         = (buf[1]>>16)&0xffff;
	padding     = (buf[1]>>14)&0x3;
	type        = (buf[1]>>8)&0x3f;
	num         = buf[1]&0xff;

	if(debug_)
	{
		cout<<"Spitting out event header data: "<<endl;
		cout<<"length: "<<length<<", tag: "<<tag<<endl;
		cout<<"padding: "<<padding<<", type: "<<type<<", num: "<<num<<endl;
	}
	int fragType = getFragType(type);
	if (fragType!=BANK) printf("data type of event should be BANK but was %d\n",type);


	parse_eventBank(&buf[fragment_offset[fragType]],length-fragment_offset[fragType]);
}

void DataReadEvio::parse_eventBank(unsigned int *buf, int bank_length) {
	int ptr = 0;
	int length,type, padding=0;
	unsigned short tag;
	unsigned short num;
	while (ptr<bank_length) {
		if (debug_) printf("ptr = %d, bank_length = %d\n",ptr,bank_length);
		length      = buf[ptr]+1;
		tag         = (buf[ptr+1]>>16)&0xffff;
		type        = (buf[ptr+1]>>8)&0x3f;
		padding     = (buf[ptr+1]>>14)&0x3;
		num         = buf[ptr+1]&0xff;
		if(debug_)
		{
			cout<<"Spitting out event bank header data: "<<endl;
			cout<<"length: "<<length<<", tag: "<<tag<<endl;
			cout<<"padding: "<<padding<<", type: "<<type<<", num: "<<num<<endl;
		}
		int fragType = getFragType(type);
		if (fragType==BANK)
		{
			switch (tag) {
				case 1:
					if (debug_) printf("ECal top bank\n");
					break;
				case 3:
					if (debug_) printf("ECal bottom bank\n");
					break;
				case 2:
					if (debug_) printf("SVT bank\n");
					parse_SVTBank(&buf[ptr+2],length-2);
					break;
				default:
					printf("Unexpected bank tag %d\n",tag);
					break;
			}
		}
		else
			if (debug_) printf("data type of event bank should be BANK but was %d\n",type);
		ptr+=length;
	}
}

void DataReadEvio::parse_SVTBank(unsigned int *buf, int bank_length) {
	int ptr = 0;
	int length,type, padding=0;
	unsigned short tag;
	unsigned short num;
	while (ptr<bank_length) {
		if (debug_) printf("ptr = %d, bank_length = %d\n",ptr,bank_length);
		length      = buf[ptr]+1;
		tag         = (buf[ptr+1]>>16)&0xffff;
		type        = (buf[ptr+1]>>8)&0x3f;
		padding     = (buf[ptr+1]>>14)&0x3;
		num         = buf[ptr+1]&0xff;
		if(debug_)
		{
			cout<<"Spitting out SVT bank header data: "<<endl;
			cout<<"length: "<<length<<", tag: "<<tag<<endl;
			cout<<"padding: "<<padding<<", type: "<<type<<", num: "<<num<<endl;
		}
		int fragType = getFragType(type);

		if (fragType==UINT32)
		{
			TrackerEvent* tb=new TrackerEvent();
			uint *data_  = (uint *)malloc((length-1) * sizeof(uint));
			memcpy(data_+1,&buf[ptr+2],(length-2)*sizeof(uint));
			data_[0] = tag;
			if (tag==7) data_[0]+=0x80000000;
			tb->copy(data_,length-1);
			free(data_);
			if(debug_){
				cout<<"Made TrackerBank for FPGA = "<<tb->fpgaAddress()<<endl;    
			}
			tee.addFPGAData(tb);
		}
		else
			printf("data type of SVT bank should be UINT32 but was %d\n",type);
		ptr+=length;
	}
}

int DataReadEvio::getFragType(int type){
	switch (type) {
		case 0x1:
			if(debug_)cout<<"found UINT32 data"<<endl;
			return(UINT32);
			break;
		case 0xe:
		case 0x10:
			if(debug_)cout<<"found BANK/ALSOBANK data"<<endl;
			return(BANK);
			break;
		case 0xf:
			if(debug_)cout<<"found COMPOSITE data"<<endl;
			return(COMPOSITE);
			break;
		case 0xd:
		case 0x20:
			if(debug_)cout<<"found SEGMENT/ALSOSEGMENT data"<<endl;
			return(SEGMENT);
			break;
		case 0xc:
			if(debug_)cout<<"found TAGSEGMENT data"<<endl;
			return(TAGSEGMENT);
			break;
		default:
			printf("?illegal fragment_type in dump_fragment: %d",type);
			exit(EXIT_FAILURE);
			break;
	}
}
