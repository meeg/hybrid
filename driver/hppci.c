//---------------------------------------------------------------------------------
// Title         : Kernel Module
// Project       : Heavy Photon PCI Card
//---------------------------------------------------------------------------------
// File          : hppci.c
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/compat.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include "HpPciMod.h"
#include "hppci.h"
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(pci, HpPci_Ids);
module_init(HpPci_Init);
module_exit(HpPci_Exit);

// Global Variable
struct HpDevice gHpDevices[MAX_PCI_DEVICES];


// Open Returns 0 on success, error code on failure
int HpPci_Open(struct inode *inode, struct file *filp) {
   struct HpDevice *hpDevice;

   // Extract structure for card
   hpDevice = container_of(inode->i_cdev, struct HpDevice, cdev);
   filp->private_data = hpDevice;

   // File is already open
   if ( hpDevice->isOpen != 0 ) {
      printk(KERN_WARNING"%s: Open: module open failed. Device is already open. Maj=%i\n",MOD_NAME,hpDevice->major);
      return ERROR;
   } else {
      hpDevice->isOpen = 1;
      return SUCCESS;
   }
}


// HpPci_Release
// Called when the device is closed
// Returns 0 on success, error code on failure
int HpPci_Release(struct inode *inode, struct file *filp) {
   struct HpDevice *hpDevice = (struct HpDevice *)filp->private_data;
   __u32 lane = 0;

   // File is not open
   if ( hpDevice->isOpen == 0 ) {
      printk(KERN_WARNING"%s: Release: module close failed. Device is not open. Maj=%i\n",MOD_NAME,hpDevice->major);
      return ERROR;
   } else {

      // Return receive buffers to hardware here
      for (lane=0; lane < 8; lane++) {
         while ( hpDevice->rxRead[lane] != hpDevice->rxWrite[lane] ) {

            // Return entry to RX queue
            hpDevice->reg->rxFree = hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->dma;

            // Increment read pointer
            hpDevice->rxRead[lane] = (hpDevice->rxRead[lane] + 1) % (hpDevice->rxBuffCnt+2);
         }
      }

      hpDevice->isOpen = 0;
      return SUCCESS;
   }
}


// HpPci_Write
// Called when the device is written to
// Returns write count on success. Error code on failure.
ssize_t HpPci_Write(struct file *filp, const char* buffer, size_t count, loff_t* f_pos) {
   struct HpDevice* hpDevice = (struct HpDevice *)filp->private_data;

   // Write ack register
   hpDevice->reg->trigAck = 0;

   return(0);
}


// HpPci_Read
// Called when the device is read from
// Returns read count on success. Error code on failure.
ssize_t HpPci_Read(struct file *filp, char *buffer, size_t count, loff_t *f_pos) {
   int         ret;
   __u32       buf[count / sizeof(__u32)];
   HpPciRx*    p64 = (HpPciRx *)buf;
   HpPciRx32*  p32 = (HpPciRx32*)buf;
   __u32       __user * dp;
   __u32       maxSize;
   __u32       copyLength;
   __u32       largeMemoryModel;
   __u32       lane;
   __u32       offset;
   __u32       eof;

   struct HpDevice *hpDevice = (struct HpDevice *)filp->private_data;

   // Copy command structure from user space
   if ( copy_from_user(buf, buffer, count) ) {
     printk(KERN_WARNING "%s: Write: failed to copy command structure from user(%p) space. Maj=%i\n",
         MOD_NAME,
         buffer,
         hpDevice->major);
     return ERROR;
   }

   largeMemoryModel = buf[0] == LargeMemoryModel;

   // Verify that size of passed structure and get variables from the correct structure.
   if ( !largeMemoryModel ) {
     // small memory model
     if ( count != sizeof(HpPciRx32) ) {
       printk(KERN_WARNING"%s: Read: passed size is not expected(%u) size(%u). Maj=%i\n",MOD_NAME, (unsigned)sizeof(HpPciRx32), (unsigned)count, hpDevice->major);
       return(ERROR);
     } else {
       dp      = (__u32*)(0LL | p32->data);
       maxSize = p32->rxMax;
       lane    = p32->rxLane;
     }
   } else {
     // large memory model
     if ( count != sizeof(HpPciRx) ) {
       printk(KERN_WARNING"%s: Read: passed size is not expected(%u) size(%u). Maj=%i\n",MOD_NAME, (unsigned)sizeof(HpPciRx), (unsigned)count, hpDevice->major);
       return(ERROR);
     } else {
       dp       = p64->data;
       maxSize  = p64->rxMax;
       lane     = p64->rxLane;
     }
   }

   // Copy settings
   if (largeMemoryModel) hpDevice->debug = p64->debugLevel;
   else hpDevice->debug = p32->debugLevel;

   // No data is ready
   if ( hpDevice->rxRead[lane] == hpDevice->rxWrite[lane] ) return(-EAGAIN);

   // Init associated data
   if (largeMemoryModel) {
     p64->eofe      = 0;
     p64->fifoErr   = 0;
     p64->lengthErr = 0;
   } else {
     p32->eofe      = 0;
     p32->fifoErr   = 0;
     p32->lengthErr = 0;
   }

   offset = 0;
   ret    = 0;
   do {

      // Wait for next data
      while ( hpDevice->rxRead[lane] == hpDevice->rxWrite[lane] );

      // User buffer is short
      if ( maxSize < ((offset/4) + (hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->length-1)) ) {
         printk(KERN_WARNING"%s: Read: user buffer is too small. Rx=%i, User=%i. Maj=%i\n",
            MOD_NAME, hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->length, maxSize, hpDevice->major);
         copyLength = maxSize;
         hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->lengthError |= 1;
      }
      else copyLength = (hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->length-1);

      // Set EOF flag
      eof = ((hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->buffer[2] >> 6) & 0x1);

      // Copy to user
      if ( copy_to_user(&(dp[offset]), &(hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->buffer[4]), copyLength*4) ) {
         printk(KERN_WARNING"%s: Read: failed to copy to user. Maj=%i\n",MOD_NAME,hpDevice->major);
         ret = ERROR;
      }
      else ret += copyLength;
      offset += (copyLength*4);

      // Copy associated data
      if (largeMemoryModel) {
        p64->rxSize    += (hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->length-1);
        p64->eofe      |= hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->eofe;
        p64->fifoErr   |= hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->fifoError;
        p64->lengthErr |= hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->lengthError;
      } else {
        p32->rxSize    += (hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->length-1);
        p32->eofe      |= hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->eofe;
        p32->fifoErr   |= hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->fifoError;
        p32->lengthErr |= hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->lengthError;
      }

      // Return entry to RX queue
      hpDevice->reg->rxFree = hpDevice->rxQueue[lane][hpDevice->rxRead[lane]]->dma;

      // Increment read pointer
      hpDevice->rxRead[lane] = (hpDevice->rxRead[lane] + 1) % (hpDevice->rxBuffCnt+2);
   } while (eof == 0);

   // Copy command structure to user space
   if ( copy_to_user(buffer, buf, count) ) {
     printk(KERN_WARNING "%s: Write: failed to copy command structure to user(%p) space. Maj=%i\n",
         MOD_NAME,
         buffer,
         hpDevice->major);
     return ERROR;
   }
   return(ret);
}


// IRQ Handler
static irqreturn_t HpPci_IRQHandler(int irq, void *dev_id, struct pt_regs *regs) {
   __u32           stat;
   __u32           descA;
   __u32           descB;
   __u32           idx;
   __u32           next;
   __u32           lane;
   irqreturn_t     ret;
   struct HpDevice *hpDevice;

   lane     = 0;
   hpDevice = (struct HpDevice *)dev_id;

   // Read IRQ Status
   stat = hpDevice->reg->irq;

   // Is this the source
   if ( (stat & 0x2) != 0 ) {

      if ( hpDevice->debug > 0 ) printk(KERN_DEBUG"%s: Irq: IRQ Called. Maj=%i\n", MOD_NAME,hpDevice->major);

      // Disable interrupts
      hpDevice->reg->irq = 0;

      // Read Rx completion status
      stat = hpDevice->reg->rxStatus;

      // Data is ready
      if ( (stat & 0x00000400) != 0 ) {

         do {

            // Read descriptor
            descA = hpDevice->reg->rxRead0;
            descB = hpDevice->reg->rxRead1;

            // Find RX buffer entry
            for ( idx=0; idx < hpDevice->rxBuffCnt; idx++ ) {
               if ( hpDevice->rxBuffer[idx]->dma == (descB & 0xFFFFFFFC) ) break;
            }

            // Entry was found
            if ( idx < hpDevice->rxBuffCnt ) {

               // Drop data if device is not open
               if ( hpDevice->isOpen ) {

                  // Setup descriptor
                  hpDevice->rxBuffer[idx]->lengthError = (descA >> 26) & 0x1;
                  hpDevice->rxBuffer[idx]->fifoError   = (descA >> 25) & 0x1;
                  hpDevice->rxBuffer[idx]->eofe        = (descA >> 24) & 0x1;
                  hpDevice->rxBuffer[idx]->length      = descA & 0x00FFFFFF;

                  // Extract lane number from data
                  lane = (hpDevice->rxBuffer[idx]->buffer[0] & 0x7);
                  hpDevice->rxBuffer[idx]->lane = lane;

                  if ( hpDevice->debug > 0 ) {
                     printk(KERN_DEBUG "%s: Irq: Rx Words=%i, Lane=%i, VC=%i, FifoErr=%i, LengthErr=%i, Addr=%p, Map=%p\n",
                        MOD_NAME, hpDevice->rxBuffer[idx]->length, hpDevice->rxBuffer[idx]->lane, 
                        hpDevice->rxBuffer[idx]->eofe, hpDevice->rxBuffer[idx]->fifoError, hpDevice->rxBuffer[idx]->lengthError, 
                        (hpDevice->rxBuffer[idx]->buffer), (void*)(hpDevice->rxBuffer[idx]->dma));
                  }

                  // Add to appropriate queue
                  next = (hpDevice->rxWrite[lane]+1) % (hpDevice->rxBuffCnt+2);
                  if ( next == hpDevice->rxRead[lane] ) printk(KERN_WARNING"%s: Irq: Rx queue pointer collision. Maj=%i\n",MOD_NAME,hpDevice->major);
                  hpDevice->rxQueue[lane][hpDevice->rxWrite[lane]] = hpDevice->rxBuffer[idx];
                  hpDevice->rxWrite[lane] = next;

               }

               // Return entry to FPGA if device is not open
               else hpDevice->reg->rxFree = (descB & 0xFFFFFFFC);

            } else printk(KERN_WARNING "%s: Irq: Failed to locate RX descriptor %.8x. Maj=%i\n",MOD_NAME,(__u32)(descA&0xFFFFFFFC),hpDevice->major);

         // Repeat while next valid flag is set
         } while ( (descB & 0x2) != 0 );
      }

      // Enable interrupts
      if ( hpDevice->debug > 0 ) printk(KERN_DEBUG"%s: Irq: Done. Maj=%i\n", MOD_NAME,hpDevice->major);
      hpDevice->reg->irq = 1;
      ret = IRQ_HANDLED;
   }
   else ret = IRQ_NONE;
   return(ret);
}


// Probe device
static int HpPci_Probe(struct pci_dev *pcidev, const struct pci_device_id *dev_id) {
   int i, res, idx;
   dev_t chrdev = 0;
   struct HpDevice *hpDevice;
   struct pci_device_id *id = (struct pci_device_id *) dev_id;

   // We keep device instance number in id->driver_data
   id->driver_data = -1;

   // Find empty structure
   for (i = 0; i < MAX_PCI_DEVICES; i++) {
      if (gHpDevices[i].baseHdwr == 0) {
         id->driver_data = i;
         break;
      }
   }

   // Overflow
   if (id->driver_data < 0) {
      printk(KERN_WARNING "%s: Probe: Too Many Devices.\n", MOD_NAME);
      return -EMFILE;
   }
   hpDevice = &gHpDevices[id->driver_data];

   // Allocate device numbers for character device.
   res = alloc_chrdev_region(&chrdev, 0, 1, MOD_NAME);
   if (res < 0) {
      printk(KERN_WARNING "%s: Probe: Cannot register char device\n", MOD_NAME);
      return res;
   }

   // Init device
   cdev_init(&hpDevice->cdev, &HpPci_Intf);

   // Initialize device structure
   hpDevice->major         = MAJOR(chrdev);
   hpDevice->cdev.owner    = THIS_MODULE;
   hpDevice->cdev.ops      = &HpPci_Intf;
   hpDevice->isOpen        = 0;
   hpDevice->debug         = 0;

   // Add device
   if ( cdev_add(&hpDevice->cdev, chrdev, 1) ) 
      printk(KERN_WARNING "%s: Probe: Error adding device Maj=%i\n", MOD_NAME,hpDevice->major);

   // Enable devices
   pci_enable_device(pcidev);

   // Get Base Address of registers from pci structure.
   hpDevice->baseHdwr = pci_resource_start (pcidev, 0);
   hpDevice->baseLen  = pci_resource_len (pcidev, 0);

   // Remap the I/O register block so that it can be safely accessed.
   hpDevice->reg = (struct HpPciReg *)ioremap_nocache(hpDevice->baseHdwr, hpDevice->baseLen);
   if (! hpDevice->reg ) {
      printk(KERN_WARNING"%s: Init: Could not remap memory Maj=%i.\n", MOD_NAME,hpDevice->major);
      return (ERROR);
   }

   // Try to gain exclusive control of memory
   if (check_mem_region(hpDevice->baseHdwr, hpDevice->baseLen) < 0 ) {
      printk(KERN_WARNING"%s: Init: Memory in use Maj=%i.\n", MOD_NAME,hpDevice->major);
      return (ERROR);
   }

   // Remove card reset, bit 1 of control register
   hpDevice->reg->control &= 0xFFFFFFFD;

   request_mem_region(hpDevice->baseHdwr, hpDevice->baseLen, MOD_NAME);
   printk(KERN_INFO "%s: Probe: Found card. Version=0x%x, Maj=%i\n", MOD_NAME,hpDevice->reg->version,hpDevice->major);

   // Get IRQ from pci_dev structure. 
   hpDevice->irq = pcidev->irq;
   printk(KERN_INFO "%s: Init: IRQ %d Maj=%i\n", MOD_NAME, hpDevice->irq,hpDevice->major);

   // Request IRQ from OS.
   if (request_irq(
       hpDevice->irq,
       HpPci_IRQHandler,
       IRQF_SHARED,
       MOD_NAME,
       (void*)hpDevice) < 0 ) {
      printk(KERN_WARNING"%s: Init: Unable to allocate IRQ. Maj=%i",MOD_NAME,hpDevice->major);
      return (ERROR);
   }

   // Set max frame size, clear rx buffer reset
   hpDevice->rxBuffSize = DEF_RX_BUF_SIZE;
   hpDevice->reg->rxMaxFrame = hpDevice->rxBuffSize | 0x80000000;

   // Init RX Buffers
   hpDevice->rxBuffCnt  = DEF_RX_BUF_CNT;
   hpDevice->rxBuffer   = (struct RxBuffer **)kmalloc(hpDevice->rxBuffCnt * sizeof(struct RxBuffer *),GFP_KERNEL);
   for(idx=0; idx < 8; idx++)
      hpDevice->rxQueue[idx] = (struct RxBuffer **)kmalloc((hpDevice->rxBuffCnt+2) * sizeof(struct RxBuffer *),GFP_KERNEL);

   for ( idx=0; idx < hpDevice->rxBuffCnt; idx++ ) {
      hpDevice->rxBuffer[idx] = (struct RxBuffer *)kmalloc(sizeof(struct RxBuffer ),GFP_KERNEL);
      if ((hpDevice->rxBuffer[idx]->buffer = pci_alloc_consistent(pcidev,hpDevice->rxBuffSize,&(hpDevice->rxBuffer[idx]->dma))) == NULL ) {
         printk(KERN_WARNING"%s: Init: unable to allocate tx buffer. Maj=%i\n",MOD_NAME,hpDevice->major);
         return ERROR;
      };

      // Add to RX queue
      hpDevice->reg->rxFree = hpDevice->rxBuffer[idx]->dma;
   }

   // Init queues
   for(idx=0; idx < 8; idx++) {
      hpDevice->rxRead[idx]  = 0;
      hpDevice->rxWrite[idx] = 0;
   }

   // Enable interrupts
   hpDevice->reg->irq = 1;

   printk(KERN_INFO"%s: Init: Driver is loaded. Maj=%i\n", MOD_NAME,hpDevice->major);
   return SUCCESS;
}


// Remove
static void HpPci_Remove(struct pci_dev *pcidev) {
   __u32 idx;
   int  i;
   struct HpDevice *hpDevice = NULL;

   // Look for matching device
   for (i = 0; i < MAX_PCI_DEVICES; i++) {
      if ( gHpDevices[i].baseHdwr == pci_resource_start(pcidev, 0)) {
         hpDevice = &gHpDevices[i];
         break;
      }
   }

   // Device not found
   if (hpDevice == NULL) {
      printk(KERN_WARNING "%s: Remove: Device Not Found.\n", MOD_NAME);
   }
   else {

      // Disable interrupts
      hpDevice->reg->irq = 0;

      // Clear RX buffer
      hpDevice->reg->rxMaxFrame = 0;

      // Free RX Buffers
      for ( idx=0; idx < hpDevice->rxBuffCnt; idx++ ) {
         pci_free_consistent(pcidev,hpDevice->rxBuffSize,hpDevice->rxBuffer[idx]->buffer,hpDevice->rxBuffer[idx]->dma);
         kfree(hpDevice->rxBuffer[idx]);
      }
      kfree(hpDevice->rxBuffer);
      for ( idx=0; idx < 8; idx++ ) kfree(hpDevice->rxQueue[idx]);

      // Set card reset, bit 1 of control register
      hpDevice->reg->control |= 0x00000002;

      // Release memory region
      release_mem_region(hpDevice->baseHdwr, hpDevice->baseLen);

      // Release IRQ
      free_irq(hpDevice->irq, hpDevice);

      // Unmap
      iounmap(hpDevice->reg);

      // Unregister Device Driver
      cdev_del(&hpDevice->cdev);
      unregister_chrdev_region(MKDEV(hpDevice->major,0), 1);

      // Disable device
      pci_disable_device(pcidev);
      hpDevice->baseHdwr = 0;
      printk(KERN_INFO"%s: Remove: Driver is unloaded. Maj=%i\n", MOD_NAME,hpDevice->major);
   }
}


// Init Kernel Module
static int HpPci_Init(void) {

   /* Allocate and clear memory for all devices. */
   memset(gHpDevices, 0, sizeof(struct HpDevice)*MAX_PCI_DEVICES);

   printk(KERN_INFO"%s: Init: HpPci Init.\n", MOD_NAME);

   // Register driver
   return(pci_register_driver(&HpPciDriver));
}


// Exit Kernel Module
static void HpPci_Exit(void) {
   printk(KERN_INFO"%s: Exit: HpPci Exit.\n", MOD_NAME);
   pci_unregister_driver(&HpPciDriver);
}


