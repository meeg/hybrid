//---------------------------------------------------------------------------------
// Title         : Kernel Module For Heavy Photon PCI Card
// Project       : Heavy Photon
//---------------------------------------------------------------------------------
// File          : hppci.h
// Author        : Ryan Herbst, rherbst@slac.stanford.edu
// Created       : 02/24/2012
//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
// Copyright (c) 2012 by SLAC National Accelerator Laboratory. All rights reserved.
//---------------------------------------------------------------------------------
// Modification history:
// 02/24/2010: created.
//---------------------------------------------------------------------------------
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/types.h>

// DMA Buffer Size, Bytes
#define DEF_RX_BUF_SIZE 8192

// Number of RX Buffers
#define DEF_RX_BUF_CNT 1024

// PCI IDs
#define PCI_VENDOR_ID_HPPCI 0x10ee
#define PCI_DEVICE_ID_HPPCI 0x0007

// Max number of devices to support
#define MAX_PCI_DEVICES 8

// Module Name
#define MOD_NAME "hppci"

enum MODELS {SmallMemoryModel=4, LargeMemoryModel=8};

// Address Map, offset from base
struct HpPciReg {
   __u32 version;     // 0x000
   __u32 scratch;     // 0x004
   __u32 irq;         // 0x008
   __u32 control;     // 0x00C

   __u32 spare0[12];  // 0x010 - 0x03C

   __u32 trigAck;     // 0x040

   __u32 spare1[15];  // 0x044 - 0x07C

   __u32 pciStat0;    // 0x080
   __u32 pciStat1;    // 0x084
   __u32 pciStat2;    // 0x088
   __u32 pciStat3;    // 0x08C

   __u32 spare2[220]; // 0x090 - 0x3FC

   __u32 rxFree;      // 0x400
   __u32 rxMaxFrame;  // 0x404
   __u32 rxStatus;    // 0x408
   __u32 rxCount;     // 0x40C

   __u32 spare3[4];   // 0x410 - 0x41C

   __u32 rxRead0;     // 0x420
   __u32 rxRead1;     // 0x424
};

// Structure for RX buffers
struct RxBuffer {
   dma_addr_t  dma;
   unchar*     buffer;
   __u32       lengthError;
   __u32       fifoError;
   __u32       eofe;
   __u32       lane;
   __u32       length;
};

// Device structure
struct HpDevice {

   // PCI address regions
   ulong             baseHdwr;
   ulong             baseLen;
   struct HpPciReg  *reg;

   // Device structure
   int         major;
   struct cdev cdev;

   // Device is already open
   __u32 isOpen;

   // Debug flag
   __u32 debug;

   // IRQ
   int irq;

   // RX/TX Buffer Structures
   __u32            rxBuffCnt;
   __u32            rxBuffSize;
   struct RxBuffer **rxBuffer;

   // Top pointer for rx queue, 2 entries larger than rxBuffCnt
   struct RxBuffer **rxQueue[8];
   __u32            rxRead[8];
   __u32            rxWrite[8];

};

// RX32 Structure
typedef struct {
   __u32   model; // large=8, small=4
   __u32   debugLevel;

   __u32   rxMax;
   __u32   data;

   __u32   rxSize;

   __u32   rxLane;

   __u32   eofe;
   __u32   fifoErr;
   __u32   lengthErr;

} HpPciRx32;

// Function prototypes
int HpPci_Open(struct inode *inode, struct file *filp);
int HpPci_Release(struct inode *inode, struct file *filp);
ssize_t HpPci_Write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
ssize_t HpPci_Read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static irqreturn_t HpPci_IRQHandler(int irq, void *dev_id, struct pt_regs *regs);
static int HpPci_Probe(struct pci_dev *pcidev, const struct pci_device_id *dev_id);
static void HpPci_Remove(struct pci_dev *pcidev);
static int HpPci_Init(void);
static void HpPci_Exit(void);

// PCI device IDs
static struct pci_device_id HpPci_Ids[] = {
   { PCI_DEVICE(PCI_VENDOR_ID_HPPCI,PCI_DEVICE_ID_HPPCI) },
   { 0, }
};

// PCI driver structure
static struct pci_driver HpPciDriver = {
  .name     = MOD_NAME,
  .id_table = HpPci_Ids,
  .probe    = HpPci_Probe,
  .remove   = HpPci_Remove,
};

// Define interface routines
struct file_operations HpPci_Intf = {
   read:    HpPci_Read,
   write:   HpPci_Write,
   open:    HpPci_Open,
   release: HpPci_Release,
};


