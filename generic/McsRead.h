//-----------------------------------------------------------------------------
// File          : McsRead.h
// Author        : Larry Ruckman  <ruckman@slac.stanford.edu>
// Created       : 10/14/2013
// Project       : Generic 
//-----------------------------------------------------------------------------
// Description :
//    Generic MCS File reader
//-----------------------------------------------------------------------------
// Copyright (c) 2013 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/14/2013: created
//-----------------------------------------------------------------------------

#ifndef __MCS_READ_H__
#define __MCS_READ_H__

#include <string>
#include <iostream>
#include <fstream>
#include <sys/types.h>

using namespace std;

#ifdef __CINT__
#define uint unsigned int
#endif

struct McsReadData {
   uint address;
   uint data;
   bool endOfFile;
} ;

//! Class to contain generic register data.
class McsRead {
   public:

      //! Constructor
      McsRead ( );

      //! Deconstructor
      ~McsRead ( );

      //! Open File
      bool open ( string filePath);

      //! Close File
      void close ( );
      
      //! Moves the ifstream to beginning of file
      void beg ( );
      
      //! Reads next byte 
      int read (McsReadData *mem); 
   
   private:
      //! Get next data record
      int next ( );   
   
      ifstream file;
      
      uint promPntr;
      uint promBaseAddr;
      uint promData[16];
      uint promAddr[16];
      
      bool endOfFile;
};
#endif
