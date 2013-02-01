//-----------------------------------------------------------------------------
// File          : Threshold.h
// Author        : Omar Moreno  <omoreno1@ucsc.edu>
// Created       : 04/14/2012
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Threshold data container.
//-----------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/14/2012: created
//-----------------------------------------------------------------------------


#include "Threshold.h"

//--- C++ ---//
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

using namespace std;

// Default constructor
Threshold::Threshold()
{
    // Instantiate the ifstream 
    inputFile = new ifstream();

    //  
    for(uint fpga = 0; fpga < Threshold::FpgaCount; fpga++){
    	for(uint hybrid = 0; hybrid  < Threshold::HybridCount; hybrid++){
		for(uint apv = 0; apv < Threshold::ApvCount; apv++){ 
			for(uint channel = 0; channel < Threshold::ChanCount; channel++){			
				threshData[fpga][hybrid][apv][channel] = 0;
			}	
		}
	}
    }
}

// Destructor
Threshold::~Threshold()
{
    delete inputFile;
}

//
void Threshold::loadThresholdData()
{
	string inputLine;
	string delimeter = ",";
	vector<string> data;
	int fpga, hybrid, apv, channel, threshold;

	// Iterate over all of the threshold values
	while( inputFile->good() ){
		
		getline(*inputFile, inputLine);

		// Skip lines which are commented out
		if(inputLine.find("%") == 0) continue;	

		// Split the line by the delimeter
		data = splitString(inputLine, delimeter);

		// If the vector contains three values, it is at the beginning of a block
		if(data.size() == 3) {
			fpga = atoi(data[0].c_str());
			hybrid = atoi(data[1].c_str());
			apv = atoi(data[2].c_str());
		}
		// If the vector contains two values, it is the channel and threshold value
		else if(data.size() == 2) {
			channel = atoi(data[0].c_str());
			threshold = atoi(data[1].c_str());
			threshData[fpga][hybrid][apv][channel%128] =
					(~THRESH_MASK & threshData[fpga][hybrid][apv][channel%128] ) | ( threshold  & THRESH_MASK);
		} else if(data.size() > 3) {
			cout << "A Data Length of " << data.size() << " is Invalid! Exiting ... " << endl;
			exit(2);
		}
		data.clear(); 
	}
	inputFile->close();

	// Create the time stamp to be stored as the threshold ID
	time_t raw_time;
	struct tm * timeinfo;

	time(&raw_time);
	timeinfo = localtime( &raw_time );

	strcpy(thresholdId, asctime(timeinfo));

	cout << "Thresholds have been set!" << endl;
}

bool Threshold::openFile( string fileName )
{
	// Open the file
	inputFile->open(fileName.c_str(), ifstream::in);
	
	// Check whether the file opened or not
        return	inputFile->is_open();
}

int Threshold::getThreshold(int fpga, int hybrid, int apv, int channel)
{
	return (threshData[fpga][hybrid][apv][channel] & THRESH_MASK);
}	

vector<string> Threshold::splitString(string s, string delimeter)
{
	size_t current_p;
	size_t next_p = -1;
	vector<string> split;
	do {
		current_p = next_p + 1;
		next_p = s.find_first_of(delimeter, current_p);
		split.push_back(s.substr(current_p, next_p - current_p));
	} while(next_p != string::npos);

	return split;
}

void Threshold::loadDefaults()
{
    // 
    for(uint fpga = 0; fpga < Threshold::FpgaCount; fpga++){
    	for(uint hybrid = 0; hybrid  < Threshold::HybridCount; hybrid++){
		for(uint apv = 0; apv < Threshold::ApvCount; apv++){ 
			for(uint channel = 0; channel < Threshold::ChanCount; channel++){			
				threshData[fpga][hybrid][apv][channel] = 0;
			}	
		}
	}
    }
    cout << "Default Thresholds have been loaded!" << endl;
	
}
