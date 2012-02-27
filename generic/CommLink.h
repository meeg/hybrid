//-----------------------------------------------------------------------------
// File          : CommLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic communications link
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __COMM_LINK_H__
#define __COMM_LINK_H__

#include <sys/types.h>
#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <CommQueue.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>

using namespace std;

class Data;
class Register;
class Command;

//! Class to contain generic communications link
class CommLink {

      // Mutux variable for thread locking
      pthread_mutex_t mutex_;

   protected:

      // Debug flag
      bool debug_;

      // Data mask
      uint dataMask_;

      // Data receive queue
      CommQueue dataQueue_;

      // Data file status
      int    dataFileFd_;
      string dataFile_;

      // Data network status
      struct sockaddr_in net_addr_;
      int                dataNetFd_;
      string             dataNetAddress_;
      int                dataNetPort_;

      // Data rx callback function
      void (*dataCb_)(void *, uint);

      // Register request/response queue
      Register *regReqEntry_;
      uint      regReqDest_;
      uint      regReqCnt_;
      bool      regReqWrite_;
      uint      regRespCnt_;

      // Command request queue
      Command  *cmdReqEntry_;
      uint      cmdReqDest_;
      uint      cmdReqCnt_;
      uint      cmdRespCnt_;

      // Run Command request queue
      Command  *runReqEntry_;
      uint      runReqDest_;
      uint      runReqCnt_;

      // Config/Status request queue
      string    cfgReqEntry_;
      bool      cfgIsConfig_;
      uint      cfgReqCnt_;
      uint      cfgRespCnt_;

      // Thread pointers
      pthread_t rxThread_;
      pthread_t ioThread_;
      pthread_t dataThread_;

      // Thread Routines
      static void *rxRun ( void *t );
      static void *ioRun ( void *t );
      static void *dataRun ( void *t );

      // Run enable
      bool runEnable_;

      // Data routine
      virtual void dataHandler();

      // Stat counters
      uint   dataFileCount_;
      uint   dataRxCount_;
      uint   regRxCount_;
      uint   timeoutCount_;
      uint   errorCount_;
      uint   unexpCount_;

      // IO handling routines
      virtual void rxHandler();
      virtual void ioHandler();

      // Max RX/Tx size
      uint maxRxTx_;

      // Buffers
      uint *rxBuff_;
      uint *txBuff_;

   public:

      //! Constructor
      CommLink ( );

      //! Deconstructor
      virtual ~CommLink ( );

      //! Open link and start threads
      /*! 
       * Return true on success.
       * Throws string on error.
      */
      virtual void open ();

      //! Stop threads and close link
      virtual void close ();

      //! Open data file
      /*! 
       * Return true on success.
       * Throws string on error.
       * \param file filename to open
      */
      void openDataFile (string file);

      //! Close data file
      void closeDataFile ();

      //! Open data network
      /*! 
       * Return true on success.
       * Throws string on error.
       * \param address network address to send data to
       * \param port    network port to send data to
      */
      void openDataNet (string address, int port);

      //! Close data network
      void closeDataNet ();

      //! Set data callback function
      /*!
       * This function is called whenever data is received.
       * The passed function accepts a data pointer and a length
       * value in bytes.
      */
      void setDataCb ( void (*dataCb_)(void *, uint));

      //! Set debug flag
      /*! 
       * \param enable Debug state
      */
      void setDebug( bool enable );

      //! Queue register request
      /*! 
       * Throws string on error.
       * \param destination Destination information
       * \param reg    Register pointer
       * \param write       Write flag
       * \param wait        Wait flag
      */
      void queueRegister ( uint destination, Register *reg, bool write, bool wait );

      //! Queue command request
      /*! 
       * Throws string on error.
       * \param destination Destination information
       * \param cmd     Command pointer
      */
      void queueCommand ( uint destination, Command *cmd );

      //! Queue run command request
      void queueRunCommand ( );

      //! Set run command
      /*! 
       * \param destination Destination information
       * \param cmd     Command pointer
      */
      void setRunCommand ( uint destination, Command *cmd );

      //! Get data file count
      uint dataFileCount ();

      //! Get data receive count
      uint   dataRxCount();

      //! Get register rx count
      uint   regRxCount();

      //! Get timeout count
      uint   timeoutCount();

      //! Get error count
      uint   errorCount();

      //! Get unexpcted count
      uint   unexpectedCount();

      //! Clear counters
      void   clearCounters();

      //! Set mask for data reception
      /*! 
       * Set mask for data reception. The mask is implementation specific.
       * \param mask Mask value
      */
      void setDataMask ( uint mask );

      //! Set max rx/tx size 
      /*! 
       * \param maxRxTx  Maximum receive/transmit size
      */
      void setMaxRxTx ( uint maxRxTx );

      //! Add configuration to data file
      /*! 
       * \param config Configuration XML data
      */
      void addConfig ( string config );

      //! Add status to data file
      /*! 
       * \param status Status XML data
      */
      void addStatus ( string status );

};

#endif

