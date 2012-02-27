//---------------------------------------------------------------------------------
// Title         : Kernel Module
// Project       : Heavy Photon PCI Card
//---------------------------------------------------------------------------------
// File          : HpPciMod.h
// Author        : Ryan Herbst, rherbst@slac.stanford.edu
// Created       : 02/24/2012
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC National Accelerator Laboratory. All rights reserved.
//---------------------------------------------------------------------------------
// Modification history:
// 02/24/2012: created.
//---------------------------------------------------------------------------------
#ifndef __HP_PCI_MOD_H__
#define __HP_PCI_MOD_H__

#include <linux/types.h>

// Return values
#define SUCCESS 0
#define ERROR   -1

// Scratchpad write value
#define SPAD_WRITE 0x55441122

// RX Structure
typedef struct {
   __u32   model;   // large=8, small=4

   // Debug level
   __u32   debugLevel;

   // Max size
   __u32   rxMax;    // dwords

   // Data pointer
   __u32*  data;

   // Data size
   __u32   rxSize;  // dwords

   // Requested lane
   __u32   rxLane;

   // Error flags
   __u32   eofe;
   __u32   fifoErr;
   __u32   lengthErr;

} HpPciRx;

#endif

