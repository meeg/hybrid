//-----------------------------------------------------------------------------
// File          : System.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic system level container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <System.h>
#include <CommLink.h>
#include <Command.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <Variable.h>
#include <time.h>
using namespace std;

// Constructor
System::System ( string name, CommLink *commLink ) : Device(0,0,name,0,NULL) {
   swRunEnable_    = false;
   swRunning_      = false;
   allStatusReq_   = false;
   topStatusReq_   = false;
   defaults_       = "defaults.xml";
   configureMsg_   = "System Is Not Configured.\nSet Defaults Or Load Settings!\n";
   swRunPeriod_    = 0;
   swRunCount_     = 0;
   swRunRetState_  = "Stopped";
   swRunError_     = "";
   errorBuffer_    = "";
   lastFileCount_  = 0;
   lastDataCount_  = 0;
   lastTime_       = 0;
   commLink_       = commLink;

   // Commands
   addCommand(new Command("SetDefaults"));
   commands_["SetDefaults"]->setDescription("Read XML defaults file.");
   commands_["SetDefaults"]->setHidden(true);

   addCommand(new Command("ReadXmlFile"));
   commands_["ReadXmlFile"]->setDescription("Read XML command or config file from disk. Pass filename as arg.");
   commands_["ReadXmlFile"]->setHidden(true);
   commands_["ReadXmlFile"]->setHasArg(true);

   addCommand(new Command("WriteConfigXml"));
   commands_["WriteConfigXml"]->setDescription("Write configuration to disk. Pass filename as arg.");
   commands_["WriteConfigXml"]->setHidden(true);
   commands_["WriteConfigXml"]->setHasArg(true);

   addCommand(new Command("WriteStatusXml"));
   commands_["WriteStatusXml"]->setDescription("Write status to disk. Pass filename as arg.");
   commands_["WriteStatusXml"]->setHidden(true);
   commands_["WriteStatusXml"]->setHasArg(true);

   addCommand(new Command("WriteStructureXml"));
   commands_["WriteStructureXml"]->setDescription("Write system structure to disk. Pass filename as arg.");
   commands_["WriteStructureXml"]->setHidden(true);
   commands_["WriteStructureXml"]->setHasArg(true);

   addCommand(new Command("OpenDataFile"));
   commands_["OpenDataFile"]->setDescription("Open data file. Pass filename as arg.");
   commands_["OpenDataFile"]->setHidden(true);

   addCommand(new Command("CloseDataFile"));
   commands_["CloseDataFile"]->setDescription("Close data file.");
   commands_["CloseDataFile"]->setHidden(true);

   addCommand(new Command("ReadConfig"));
   commands_["ReadConfig"]->setDescription("Read configuration.");
   commands_["ReadConfig"]->setHidden(true);

   addCommand(new Command("ReadStatus"));
   commands_["ReadStatus"]->setDescription("Read status.");
   commands_["ReadStatus"]->setHidden(true);

   addCommand(new Command("VerifyConfig"));
   commands_["VerifyConfig"]->setDescription("Verify configuration");
   commands_["VerifyConfig"]->setHidden(true);

   addCommand(new Command("ResetCount"));
   commands_["ResetCount"]->setDescription("Reset top level counters.");
   commands_["ResetCount"]->setHidden(true);

   addCommand(new Command("SetRunState"));
   commands_["SetRunState"]->setDescription("Set run state");
   commands_["SetRunState"]->setHidden(true);

   addCommand(new Command("HardReset"));
   commands_["HardReset"]->setDescription("Hard reset System.");
   commands_["HardReset"]->setHidden(true);

   addCommand(new Command("SoftReset"));
   commands_["SoftReset"]->setDescription("Soft reset System.");
   commands_["SoftReset"]->setHidden(true);

   addCommand(new Command("RefreshState"));
   commands_["RefreshState"]->setDescription("Refresh System State.");
   commands_["RefreshState"]->setHidden(true);

   // Variables
   addVariable(new Variable("DataFileCount",Variable::Status));
   variables_["DataFileCount"]->setDescription("Number of events written to the data file");
   variables_["DataFileCount"]->setHidden(true);

   addVariable(new Variable("DataFile",Variable::Configuration));
   variables_["DataFile"]->setDescription("Data File For Write");
   variables_["DataFile"]->setHidden(true);

   addVariable(new Variable("DataRxCount",Variable::Status));
   variables_["DataRxCount"]->setDescription("Number of events received");
   variables_["DataRxCount"]->setHidden(true);

   addVariable(new Variable("RegRxCount",Variable::Status));
   variables_["RegRxCount"]->setDescription("Number of register responses received");
   variables_["RegRxCount"]->setHidden(true);

   addVariable(new Variable("UnexpectedCount",Variable::Status));
   variables_["UnexpectedCount"]->setDescription("Number of unexpected receive packets");
   variables_["UnexpectedCount"]->setHidden(true);

   addVariable(new Variable("TimeoutCount",Variable::Status));
   variables_["TimeoutCount"]->setDescription("Number of timeout errors");
   variables_["TimeoutCount"]->setHidden(true);

   addVariable(new Variable("ErrorCount",Variable::Status));
   variables_["ErrorCount"]->setDescription("Number of errors");
   variables_["ErrorCount"]->setHidden(true);

   addVariable(new Variable("DataOpen",Variable::Status));
   variables_["DataOpen"]->setDescription("Data file is open");
   variables_["DataOpen"]->setTrueFalse();
   variables_["DataOpen"]->setHidden(true);

   addVariable(new Variable("RunRate",Variable::Configuration));
   variables_["RunRate"]->setDescription("Run rate");
   variables_["RunRate"]->setHidden(true);
   vector<string> rates;
   rates.resize(4);
   rates[0] = "1Hz";
   rates[1] = "10Hz";
   rates[2] = "100Hz";
   rates[3] = "120Hz";
   variables_["RunRate"]->setEnums(rates);

   addVariable(new Variable("RunCount",Variable::Configuration));
   variables_["RunCount"]->setDescription("SW Run Count");
   variables_["RunCount"]->setHidden(true);
   variables_["RunCount"]->setInt(1000);

   addVariable(new Variable("RunState",Variable::Status));
   variables_["RunState"]->setDescription("Run state");
   variables_["RunState"]->setHidden(true);
   vector<string> states;
   states.resize(2);
   states[0] = "Stopped";
   states[1] = "Running";
   variables_["RunState"]->setEnums(states);

   addVariable(new Variable("RunProgress",Variable::Status));
   variables_["RunProgress"]->setDescription("Run Total");
   variables_["RunProgress"]->setHidden(true);

   addVariable(new Variable("DebugEnable",Variable::Configuration));
   variables_["DebugEnable"]->setDescription("Enable console debug messages.");
   variables_["DebugEnable"]->setTrueFalse();
   variables_["DebugEnable"]->set("True");

   addVariable(new Variable("DebugCmdTime",Variable::Configuration));
   variables_["DebugCmdTime"]->setDescription("Enable showing command execution time.");
   variables_["DebugCmdTime"]->setTrueFalse();
   variables_["DebugCmdTime"]->set("True");

   addVariable(new Variable("SystemState",Variable::Status));
   variables_["SystemState"]->setDescription("Current system state.");
   variables_["SystemState"]->setHidden(true);

   variables_["enabled"]->setHidden(true);
}

// Deconstructor
System::~System ( ) {
}


// Set comm link
CommLink * System::commLink() {
   return(commLink_);
}

// Thread Routines
void *System::swRunStatic ( void *t ) {
   System *ti;
   ti = (System *)t;
   ti->swRunThread();
   pthread_exit(NULL);
}

void System::swRunThread() {
   struct timespec tme;
   ulong           ctime;
   ulong           ltime;
   uint            runTotal;

   swRunning_ = true;
   swRunError_  = "";
   clock_gettime(CLOCK_REALTIME,&tme);
   ltime = (tme.tv_sec * 1000000) + (tme.tv_nsec/1000);

   // Get run attributes
   runTotal  = 0;

   if ( debug_ ) {
      cout << "System::runThread -> Name: " << name_ 
           << ", Run Started"
           << ", RunCount=" << dec << swRunCount_
           << ", RunPeriod=" << dec << swRunPeriod_ << endl;
   }

   // Run
   while ( swRunEnable_ && runTotal < swRunCount_ ) {

      // Delay
      do {
         usleep(1);
         clock_gettime(CLOCK_REALTIME,&tme);
         ctime = (tme.tv_sec * 1000000) + (tme.tv_nsec/1000);
      } while ( (ctime-ltime) < swRunPeriod_);

      // Execute command
      ltime = ctime;
      commLink_->queueRunCommand();
      runTotal++;
      variables_["RunProgress"]->setInt((uint)(((double)runTotal/(double)swRunCount_)*100.0));
   }

   if ( debug_ ) {
      cout << "System::runThread -> Name: " << name_ 
           << ", Run Stopped, RunTotal = " << dec << runTotal << endl;
   }

   // Set run
   sleep(1);
   variables_["RunProgress"]->setInt((uint)(((double)runTotal/(double)swRunCount_)*100.0));
   variables_["RunState"]->set(swRunRetState_);
   swRunning_ = false;
   pthread_exit(NULL);
}

// Start Run
void System::setRunState(string state) {
   stringstream err;
   uint         toCount;

   // Stopped state is requested
   if ( state == "Stopped" ) swRunEnable_ = false;

   // Running state is requested
   else if ( state == "Running" && !swRunning_ ) {
      swRunRetState_ = get("RunState");
      swRunEnable_   = true;
      variables_["RunState"]->set("Running");

      // Setup run parameters
      swRunCount_ = getInt("RunCount");
      if      ( get("RunRate") == "120Hz") swRunPeriod_ =    8333;
      else if ( get("RunRate") == "100Hz") swRunPeriod_ =   10000;
      else if ( get("RunRate") ==  "10Hz") swRunPeriod_ =  100000;
      else if ( get("RunRate") ==   "1Hz") swRunPeriod_ = 1000000;
      else swRunPeriod_ = 1000000;

      // Set run command here when re-implemented
      //device->setRuncommand("command");

      // Start thread
      if ( swRunCount_ > 0 && pthread_create(&swRunThread_,NULL,swRunStatic,this) ) {
         err << "System::startRun -> Failed to create runThread" << endl;
         if ( debug_ ) cout << err.str();
         variables_["RunState"]->set(swRunRetState_);
         throw(err.str());
      }

      // Wait for thread to start
      toCount = 0;
      while ( !swRunning_ ) {
         usleep(100);
         toCount++;
         if ( toCount > 1000 ) {
            swRunEnable_ = false;
            err << "System::startRun -> Timeout waiting for runthread" << endl;
            if ( debug_ ) cout << err.str();
            variables_["RunState"]->set(swRunRetState_);
            throw(err.str());
         }
      }
   }
}

// Method to process a command
void System::command ( string name, string arg ) {
   ofstream     os;
   stringstream tmp;
   struct timespec stme;
   struct timespec etme;

   clock_gettime(CLOCK_REALTIME,&stme);

   // Read defaults file
   if ( name == "SetDefaults" ) parseXmlFile(defaults_);

   // Read and parse xml file
   else if ( name == "ReadXmlFile" ) parseXmlFile(arg);

   // Write config xml dump
   else if ( name == "WriteConfigXml" ) {
      readConfig();
      os.open(arg.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "System::command -> Error opening config xml file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }
      os << "<system>" << endl << configString(true) << "</system>" << endl;
      os.close();
   }

   // Write status xml dump
   else if ( name == "WriteStatusXml" ) {
      readStatus();
      os.open(arg.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "System::command -> Error opening status xml file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }
      os << "<system>" << endl << statusString(true) << "</system>" << endl;
      os.close();
   }

   // Write structure xml dump
   else if ( name == "WriteStructureXml" ) {
      os.open(arg.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "System::command -> Error opening structure xml file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }
      os << structureString(true) << endl;
      os.close();
   }

   // Open data file
   else if ( name == "OpenDataFile" ) {
      command("CloseDataFile","");
      commLink_->openDataFile(variables_["DataFile"]->get());
      commLink_->addConfig(configString(true));
      readStatus();
      commLink_->addStatus(statusString(true));
      variables_["DataOpen"]->set("True");
   }

   // Close data file
   else if ( name == "CloseDataFile" ) {
      if ( get("DataOpen") == "True" ) {
         readStatus();
         commLink_->addStatus(statusString(true));
         commLink_->closeDataFile();
         variables_["DataOpen"]->set("False");
      }
   }

   // Send config xml
   else if ( name == "ReadConfig" ) {
      readConfig();
      allConfigReq_ = true;
   }

   // Send status xml
   else if ( name == "ReadStatus" ) allStatusReq_ = true;

   // Send verify status
   else if ( name == "VerifyConfig" ) {
      verifyConfig();
      allStatusReq_ = true;
   }

   // Reset counters
   else if ( name == "ResetCount" ) commLink_->clearCounters();

   // Start Run
   else if ( name == "SetRunState" ) setRunState(arg);

   // Hard reset
   else if ( name == "HardReset" ) hardReset();

   // Soft reset
   else if ( name == "SoftReset" ) softReset();

   else if ( name == "RefreshState" ) allStatusReq_ = true;

   else Device::command(name,arg);

   clock_gettime(CLOCK_REALTIME,&etme);

   if ( get("DebugCmdTime") == "True" ) {
      cout << "System::command -> Command " << name << " time results: " << endl
           << "   Start Time: " << dec << stme.tv_sec << "." << stme.tv_nsec << endl
           << "     End Time: " << dec << etme.tv_sec << "." << etme.tv_nsec << endl;
   }
}

// Parse XML string
bool System::parseXml ( string xml, bool force ) {
   xmlDocPtr    doc;
   xmlNodePtr   node;
   xmlNodePtr   childNode;
   const char   *childName;
   string       err;
   string       stat;
   bool         configUpdate;

   stat = "";
   configUpdate = false;
   try {

      // Parse string
      doc = xmlReadMemory(xml.c_str(), strlen(xml.c_str()), "string.xml", NULL, 0);
      if (doc == NULL) {
         err = "System::parseXml -> Failed to parse string\n";
         if ( debug_ ) cout << err;
         throw(err);
      }

      // get the root element node
      node = xmlDocGetRootElement(doc);

      // Look for child nodes
      for ( childNode = node->children; childNode; childNode = childNode->next ) {
         if ( childNode->type == XML_ELEMENT_NODE ) {
            childName  = (const char *)childNode->name;

            // Config
            if ( strcmp(childName,"config") == 0 ) {
               if ( setXmlConfig(childNode) ) {
                  writeConfig(force);
                  if ( force ) verifyConfig();
                  configUpdate = true;
               }
            }

            // Command
            else if ( strcmp(childName,"command") == 0 ) execXmlCommand(childNode);
         }
      }
   } catch ( string error ) { stat = error; }

   // Cleanup
   xmlFreeDoc(doc);
   xmlCleanupParser();
   xmlMemoryDump();

   if ( stat != "" ) throw(stat);
   return(configUpdate);
}


// Parse XML string
void System::parseXmlString ( string xml ) {
   try { 
      if ( parseXml(xml,false) ) allConfigReq_ = true;
   } catch ( string error ) { 
      errorBuffer_.append("<error>");
      errorBuffer_.append(error); 
      errorBuffer_.append("</error>\n");
      configureMsg_ = "A System Error Has Occured!\n";
      configureMsg_.append("Please HardReset and then configure!\n");
   }
   topStatusReq_ = true;
}

// Parse XML file
void System::parseXmlFile ( string file ) {
   uint         idx;
   ifstream     is;
   stringstream tmp;
   stringstream buffer;
  
   // Stop run and close file
   setRunState("Stopped");
   command("CloseDataFile","");
 
   // Open file
   is.open(file.c_str());
   if ( ! is.is_open() ) {
      tmp.str("");
      tmp << "System::parseXmlFile -> Error opening xml file for read: " << file << endl;
      if ( debug_ ) cout << tmp.str();
      throw(tmp.str());
   }
   buffer.str("");
   buffer << is.rdbuf();
   is.close();

   // Parse string
   parseXml(buffer.str(),true);

   // Update message
   configureMsg_ = "System Configured From ";
   idx = file.find_last_of("/");
   configureMsg_.append(file.substr(idx+1));
   configureMsg_.append(".\n");
   allStatusReq_ = true;
   allConfigReq_ = true;
}

//! Method to perform soft reset
void System::softReset ( ) { 
   setRunState("Stopped");
   command("CloseDataFile","");
   allStatusReq_ = true;
}

//! Method to perform hard reset
void System::hardReset ( ) { 
   configureMsg_ = "System Is Not Configured.\nSet Defaults Or Load Settings!\n";
   setRunState("Stopped");
   command("CloseDataFile","");
   topStatusReq_ = true;
}

//! return local state
string System::localState () {
   return("");
}

//! Method to return state string
string System::poll () {
   uint         curr;
   uint         rate;
   stringstream msg;
   time_t       currTime;
   bool         send;
   string       stateIn;

   time(&currTime);

   // Update debug state
   if ( debug_ == true && get("DebugEnable") == "False" ) {
      cout << "System::writeConfig -> Name: " << name_ << " Disabling debug messages." << endl;
      commLink_->setDebug(false);
      setDebug(0);
   }
   else if ( debug_ == false && get("DebugEnable") == "True" ) {
      cout << "System::writeConfig -> Name: " << name_ << " Enabling debug messages." << endl;
      commLink_->setDebug(true);
      setDebug(1);
   }

   // Detect run stop
   if ( swRunEnable_ && !swRunning_ ) {
      swRunEnable_ = false;
      allStatusReq_ = true;
      if ( swRunError_ != "" ) {
         errorBuffer_.append("<error>");
         errorBuffer_.append(swRunError_); 
         errorBuffer_.append("</error>\n");
         configureMsg_ = "A System Error Has Occured!\n";
         configureMsg_.append("Please HardReset and then configure!\n");
      }
   }

   try { 

      // Read status if requested
      if ( allStatusReq_ ) readStatus();

      // Local status
      stateIn = localState();

   } catch (string error ) {
      errorBuffer_.append("<error>");
      errorBuffer_.append(error); 
      errorBuffer_.append("</error>\n");
      configureMsg_ = "A System Error Has Occured!\n";
      configureMsg_.append("Please HardReset and then configure!\n");
   }

   // Update state message
   msg.str("");
   msg << configureMsg_;
   msg << "System is is in run state '" << get("RunState") << "'" << endl;
   msg << stateIn;
   variables_["SystemState"]->set(msg.str());

   // Once a second updates
   if ( currTime != lastTime_ ) {
      lastTime_ = currTime;

      // File counters
      variables_["RegRxCount"]->setInt(commLink_->regRxCount());
      variables_["TimeoutCount"]->setInt(commLink_->timeoutCount());
      variables_["ErrorCount"]->setInt(commLink_->errorCount());
      variables_["UnexpectedCount"]->setInt(commLink_->unexpectedCount());

      curr = commLink_->dataFileCount();
      if ( curr < lastFileCount_ ) rate = 0;
      else rate = curr - lastFileCount_;
      lastFileCount_ = curr;
      msg.str("");
      msg << dec << curr << " - " << dec << rate << " Hz";
      variables_["DataFileCount"]->set(msg.str());
   
      curr = commLink_->dataRxCount();
      if ( curr < lastDataCount_ ) rate = 0;
      else rate = curr - lastDataCount_;
      lastDataCount_ = curr;
      msg.str("");
      msg << dec << curr << " - " << dec << rate << " Hz";
      variables_["DataRxCount"]->set(msg.str());

      // Top status once a second
      topStatusReq_ = true;
   }

   // Generate outgoing message
   send = false;
   msg.str("");
   msg << "<system>" << endl;
   if ( errorBuffer_ != "" ) { msg << errorBuffer_; send = true; }
   if ( topStatusReq_ || allStatusReq_ ) { msg << statusString(false); send=true; }
   if ( allConfigReq_ ) { msg << configString(false); send=true; }
   msg << "</system>" << endl;

   // Do we add configuration updates to file?
   if ( allConfigReq_ ) commLink_->addConfig(configString(true));
   if ( allStatusReq_ || allConfigReq_ ) commLink_->addStatus(statusString(true));

   // Clear send requests
   errorBuffer_ = "";
   topStatusReq_ = false; 
   allStatusReq_ = false; 
   allConfigReq_ = false; 

   // Send message
   if ( send ) return(msg.str());
   else return("");
}

// Return status string
string System::statusString(bool hidden) {
   stringstream tmp;
   tmp << "<status>" << endl;
   tmp << getXmlStatus(true,hidden);
   tmp << "</status>" << endl;
   return(tmp.str());
}

// Return config string
string System::configString(bool hidden) {
   stringstream tmp;
   tmp << "<config>" << endl;
   tmp << getXmlConfig(true,true,hidden);  // Common
   tmp << getXmlConfig(true,false,hidden); // Per-Instance
   tmp << "</config>" << endl;
   return(tmp.str());
}

// Return structure string
string System::structureString (bool hidden) {
   stringstream tmp;
   tmp << "<system>" << endl;
   tmp << "<structure>" << endl;
   tmp << getXmlStructure(true,true,hidden);  // General
   tmp << getXmlStructure(true,false,hidden); // Per-Instance
   tmp << "</structure>" << endl;
   tmp << "</system>" << endl;
   return(tmp.str());
}

