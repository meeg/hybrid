//-----------------------------------------------------------------------------
// File          : DataRead.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// adapted by mgraham on 4/2/2012 to read in evio files
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
#ifndef __DATA_READ_EVIO_H__
#define __DATA_READ_EVIO_H__

#include <evio.h>
#include <string>
#include <map>
#include <TrackerEvioEvent.h>
#include <sys/types.h>
#include <DataRead.h>
using namespace std;
#define MAXEVIOBUF   1000000

// Define variable holder
typedef map<string,string> VariableHolder;

//! Class to contain generic register data.
class DataReadEvio : public DataRead {
  bool debug_;

  TrackerEvioEvent tee;
  int tee_fpga;
      //
      int maxbuf ;
      
      // Strip whitespace
      string removeWhite ( string str );


      void parse_fragment( unsigned int *buf, int fragment_type,TrackerEvioEvent* tee)  ;
      
      void makeTrackerBanks(unsigned int *data, int type, int length, int padding, int tag,TrackerEvioEvent* tee) ;
      
      int getFragType(int);

      void eventInfo(unsigned int *buf);

      enum {
	BANK = 0,
	SEGMENT,
	TAGSEGMENT,
      };
      int fragment_offset[3];
      int depth;
      bool inSVT;
      int nbanks;
      /* formatting info */
      const static int xtod         = 0;
      const static int n8           = 8;
      const static int n16          = 8;
      const static int n32          = 5;
      const static int n64          = 2;
      const static int w8           = 4;
      const static int w16          = 9;
      const static int w32          = 14;
      const static int p32          = 6;
      const static int w64          = 28;
      const static int p64          = 20;

      int evtType;
      unsigned short evtTag;
      unsigned char evtNum;
      int position;

   public:
     
      //! Constructor
      DataReadEvio ( );

      //! Deconstructor
      ~DataReadEvio ( );

      bool open ( string file);

      //! Get next data record
      /*! 
       * Returns true on success
       * \param data Data object to store data
      */
      bool  next ( Data *data);
      
      //! Get next data record & create new data object
      /*! 
       * Returns NULL on failure
      */
      //      Data *next ( );

};
#endif
