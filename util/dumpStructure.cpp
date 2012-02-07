//-----------------------------------------------------------------------------
// File          : dumpStructure.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon API
//-----------------------------------------------------------------------------
// Description :
// Devel board test code
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//----------------------------------------------------------------------------
#include <Tracker.h>
#include <iomanip>
#include <fstream>
#include <iostream>
using namespace std;

int main (int argc, char **argv) {
   Tracker *tracker = new Tracker(NULL);
   tracker->command("WriteStructureXml","doc/structure.xml");
}

