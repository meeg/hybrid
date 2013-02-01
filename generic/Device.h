//-----------------------------------------------------------------------------
// File          : Device.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic device container.
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <libxml/tree.h>
#include <pthread.h>
using namespace std;

class Variable;
class Register;
class Command;
class Device;
class CommLink;
class System;

// Define local types
typedef map<string,Variable *>    VariableMap;
typedef map<string,Register *>    RegisterMap;
typedef map<string,Command  *>    CommandMap;
typedef vector<Device *>          DeviceVector;
typedef map<string,DeviceVector*> DeviceMap;

// Macro to create lock and start try block
#define REGISTER_LOCK pthread_mutex_lock(&mutex_); try {

// Macro to remove lock and end try block. Errors are caught and re-thrown
#define REGISTER_UNLOCK } catch (string error) { \
                           pthread_mutex_unlock(&mutex_); \
                           throw(error); \
                        } pthread_mutex_unlock(&mutex_); 

//! Class to contain generic device data.
class Device {

   protected:

      // Mutux variable for thread locking
      pthread_mutex_t mutex_;

      // Device destination
      uint destination_;

      // Device base address
      uint baseAddress_;

      // Device name
      string name_;

      // Device index
      uint index_;

      // Map of variables
      VariableMap variables_;

      // Map of registers
      RegisterMap registers_;

      // Map of commands
      CommandMap commands_;

      // Map of device vectors
      DeviceMap devices_;

      // Description
      string desc_;

      // Debug flag
      bool debug_;

      // Parent device & top system
      Device *parent_;
      System *system_;

      // Write register if stale or if force = true
      // Throws string on error
      void writeRegister ( Register *reg, bool force, bool wait=true );

      // Read register
      // Throws string on error
      void readRegister ( Register *reg );

      // Verify register
      // Throws string on verify fail
      void verifyRegister ( Register *reg, bool warnOnly = false );

      // Method to set variable values from xml tree
      bool setXmlConfig ( xmlNode *node );

      // Method to get config variable values in xml form.
      // Two different types of config variables can be returned
      // per-device variables and common variables. The common flag
      // determines which type is to be returned. The hidden flag
      // determines if hidden variables should be included in the
      // string. Hidden variables will always be sent for the top
      // level device, determine by the top flag.
      string getXmlConfig ( bool top, bool common, bool hidden, uint level );

      // Method to get status variable values in xml form.
      // The hidden flag determines if hidden status variables should 
      // be included in the string. Hidden variables will always be sent 
      // for the top level device, determine by the top flag.
      string getXmlStatus (bool top, bool hidden, uint level );

      // Method to execute commands from xml tree
      // Throws string on error
      void execXmlCommand ( xmlNode *node );

      // Method to get device structure in xml form.
      // Two different types of variables and commands can be returned
      // per-device and common The common flag determines which type 
      // is to be returned. The hidden flag determines if hidden entries 
      // should be included in the string. Hidden values will always be 
      // sent for the top level device, determine by the top flag.
      string getXmlStructure ( bool top, bool common, bool hidden, uint level);

      // Add registers
      void addRegister(Register *reg);

      // Add variables
      void addVariable(Variable *variable);

      // Add devices
      void addDevice(Device *device);

      // Add commands
      void addCommand(Command *cmd);

      // Return register, throws exception when not found
      Register *getRegister(string name);

      // Return variable, throw exception when not found
      Variable *getVariable(string name);

      // Return command, throw exception when not found
      Command *getCommand(string name);

   public:

      //! Constructor
      /*! 
       * \param destination Device destination
       * \param baseAddress Device base address
       * \param name        Device name
       * \param index       Device index
       * \param parent      Parent device
      */
      Device ( uint destination, uint baseAddress, string name, uint index, Device *parent );

      //! Deconstructor
      virtual ~Device ( );

      //! Set debug flag
      /*! 
       * \param enable    Debug state
      */
      void setDebug( bool enable );

      //! Method to get name
      string name ();

      //! Method to get index
      uint index();

      //! Method to get destination
      uint destination();

      //! Method to get base address
      uint baseAddress();

      //! Method to get sub device
      /*!
       * Throws string if device can't be found
       * \param name Device name
       * \param index Device index
      */
      Device * device ( string name, uint index = 0 );

      //! Method to process a command
      /*!
       * \param name     Command name
       * \param arg      Optional arg
      */
      virtual void command ( string name, string arg );

      //! Method to set command for running
      /*!
       * \param name     Command name
      */
      void setRunCommand ( string name );

      //! Method to set a single variable
      /*!
       * Throws string on error
       * \param variable Variable name
       * \param value    Variable value
      */
      void set ( string variable, string value );

      //! Method to get a single variable
      /*! 
       * Return variable value
       * Throws string on error
       * \param variable Variable name
      */
      string get ( string variable );

      //! Method to set a single variable, integer value
      /*!
       * Throws string on error
       * \param variable Variable name
       * \param value    Variable value
      */
      void setInt ( string variable, uint value );

      //! Method to get a single variable, integer value
      /*! 
       * Return variable value
       * Throws string on error
       * \param variable Variable name
      */
      uint getInt ( string variable );

      //! Method to read a specific register
      /*! 
       * Read a specific register without updaing associated variables.
       * Used for debug purposes.
       * Throws string on error.
       * \param name Register name
      */
      uint readSingle ( string name );

      //! Method to write a specific register
      /*! 
       * Write a specific register ignoring associated variable values
       * Used for debug purposes.
       * Throws string on error.
       * \param name Register name
       * \param value Register value
      */
      void writeSingle( string name, uint value );

      //! Method to read status registers and update variables
      /*! 
       * Throws string on error.
      */
      virtual void readStatus ( );

      //! Method to read configuration registers and update variables
      /*! 
       * Throws string on error.
      */
      virtual void readConfig ( );

      //! Method to write configuration registers
      /*! 
       * Throws string on error.
       * \param force Write all registers if true, only stale if false
      */
      virtual void writeConfig ( bool force );

      //! Verify hardware state of configuration
      virtual void verifyConfig ( );

};
#endif
