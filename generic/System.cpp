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
#include <ControlCmdMem.h>
using namespace std;

// Constructor
System::System ( string name, CommLink *commLink ) : Device(0,0,name,0,NULL) {
   swRunEnable_    = false;
   swRunning_      = false;
   hwRunning_      = false;
   allStatusReq_   = false;
   topStatusReq_   = false;
   defaults_       = "defaults.xml";
   configureMsg_   = "System Is Not Configured.\nSet Defaults Or Load Settings!\n";
   swRunPeriod_    = 0;
   swRunCount_     = 0;
   swRunRetState_  = "Stopped";
   swRunError_     = "";
   errorBuffer_    = "";
   errorFlag_      = false;
   lastFileCount_  = 0;
   lastDataCount_  = 0;
   lastTime_       = 0;
   commLink_       = commLink;

   xmlInitParser();

   // Commands
   addCommand(new Command("SetDefaults"));
   getCommand("SetDefaults")->setDescription("Read XML defaults file.");
   getCommand("SetDefaults")->setHidden(true);

   addCommand(new Command("ReadXmlFile"));
   getCommand("ReadXmlFile")->setDescription("Read XML command or config file from disk. Pass filename as arg.");
   getCommand("ReadXmlFile")->setHidden(true);
   getCommand("ReadXmlFile")->setHasArg(true);

   addCommand(new Command("WriteConfigXml"));
   getCommand("WriteConfigXml")->setDescription("Write configuration to disk. Pass filename as arg.");
   getCommand("WriteConfigXml")->setHidden(true);
   getCommand("WriteConfigXml")->setHasArg(true);

   addCommand(new Command("WriteStatusXml"));
   getCommand("WriteStatusXml")->setDescription("Write status to disk. Pass filename as arg.");
   getCommand("WriteStatusXml")->setHidden(true);
   getCommand("WriteStatusXml")->setHasArg(true);

   addCommand(new Command("WriteStructureXml"));
   getCommand("WriteStructureXml")->setDescription("Write system structure to disk. Pass filename as arg.");
   getCommand("WriteStructureXml")->setHidden(true);
   getCommand("WriteStructureXml")->setHasArg(true);

   addCommand(new Command("OpenDataFile"));
   getCommand("OpenDataFile")->setDescription("Open data file.");
   getCommand("OpenDataFile")->setHidden(true);

   addCommand(new Command("CloseDataFile"));
   getCommand("CloseDataFile")->setDescription("Close data file.");
   getCommand("CloseDataFile")->setHidden(true);

   addCommand(new Command("ReadConfig"));
   getCommand("ReadConfig")->setDescription("Read configuration.");
   getCommand("ReadConfig")->setHidden(true);

   addCommand(new Command("ReadStatus"));
   getCommand("ReadStatus")->setDescription("Read status.");
   getCommand("ReadStatus")->setHidden(true);

   addCommand(new Command("VerifyConfig"));
   getCommand("VerifyConfig")->setDescription("Verify configuration");
   getCommand("VerifyConfig")->setHidden(true);

   addCommand(new Command("ResetCount"));
   getCommand("ResetCount")->setDescription("Reset top level counters.");
   getCommand("ResetCount")->setHidden(true);

   addCommand(new Command("SetRunState"));
   getCommand("SetRunState")->setDescription("Set run state");
   getCommand("SetRunState")->setHidden(true);

   addCommand(new Command("HardReset"));
   getCommand("HardReset")->setDescription("Hard reset System.");
   getCommand("HardReset")->setHidden(true);

   addCommand(new Command("SoftReset"));
   getCommand("SoftReset")->setDescription("Soft reset System.");
   getCommand("SoftReset")->setHidden(true);

   addCommand(new Command("RefreshState"));
   getCommand("RefreshState")->setDescription("Refresh System State.");
   getCommand("RefreshState")->setHidden(true);

   // Variables
   addVariable(new Variable("DataFileCount",Variable::Status));
   getVariable("DataFileCount")->setDescription("Number of events written to the data file");
   getVariable("DataFileCount")->setHidden(true);

   addVariable(new Variable("DataFile",Variable::Configuration));
   getVariable("DataFile")->setDescription("Data File For Write");
   getVariable("DataFile")->setHidden(true);

   addVariable(new Variable("DataCompress",Variable::Configuration));
   getVariable("DataCompress")->setDescription("Compress Data Files");
   getVariable("DataCompress")->setTrueFalse();
   getVariable("DataCompress")->setHidden(true);   

   addVariable(new Variable("DataRxCount",Variable::Status));
   getVariable("DataRxCount")->setDescription("Number of events received");
   getVariable("DataRxCount")->setHidden(true);

   addVariable(new Variable("RegRxCount",Variable::Status));
   getVariable("RegRxCount")->setDescription("Number of register responses received");
   getVariable("RegRxCount")->setHidden(true);

   addVariable(new Variable("UnexpectedCount",Variable::Status));
   getVariable("UnexpectedCount")->setDescription("Number of unexpected receive packets");
   getVariable("UnexpectedCount")->setHidden(true);

   addVariable(new Variable("TimeoutCount",Variable::Status));
   getVariable("TimeoutCount")->setDescription("Number of timeout errors");
   getVariable("TimeoutCount")->setHidden(true);

   addVariable(new Variable("ErrorCount",Variable::Status));
   getVariable("ErrorCount")->setDescription("Number of errors");
   getVariable("ErrorCount")->setHidden(true);

   addVariable(new Variable("DataOpen",Variable::Status));
   getVariable("DataOpen")->setDescription("Data file is open");
   getVariable("DataOpen")->setTrueFalse();
   getVariable("DataOpen")->setHidden(true);

   addVariable(new Variable("RunRate",Variable::Configuration));
   getVariable("RunRate")->setDescription("Run rate");
   getVariable("RunRate")->setHidden(true);
   vector<string> rates;
   rates.resize(6);
   rates[0] = "1Hz";
   rates[1] = "10Hz";
   rates[2] = "100Hz";
   rates[3] = "120Hz";
   rates[4] = "1000Hz";
   rates[5] = "2000Hz";
   getVariable("RunRate")->setEnums(rates);

   addVariable(new Variable("RunCount",Variable::Configuration));
   getVariable("RunCount")->setDescription("SW Run Count");
   getVariable("RunCount")->setHidden(true);
   getVariable("RunCount")->setInt(1000);

   addVariable(new Variable("RunState",Variable::Status));
   getVariable("RunState")->setDescription("Run state");
   getVariable("RunState")->setHidden(true);
   vector<string> states;
   states.resize(2);
   states[0] = "Stopped";
   states[1] = "Running";
   getVariable("RunState")->setEnums(states);

   addVariable(new Variable("RunProgress",Variable::Status));
   getVariable("RunProgress")->setDescription("Run Total");
   getVariable("RunProgress")->setHidden(true);

   addVariable(new Variable("DebugEnable",Variable::Configuration));
   getVariable("DebugEnable")->setDescription("Enable console debug messages.");
   getVariable("DebugEnable")->setTrueFalse();
   getVariable("DebugEnable")->set("False");

   addVariable(new Variable("DebugCmdTime",Variable::Configuration));
   getVariable("DebugCmdTime")->setDescription("Enable showing command execution time.");
   getVariable("DebugCmdTime")->setTrueFalse();
   getVariable("DebugCmdTime")->set("True");

   addVariable(new Variable("SystemState",Variable::Status));
   getVariable("SystemState")->setDescription("Current system state.");
   getVariable("SystemState")->setHidden(true);

   addVariable(new Variable("UserStatus",Variable::Status));
   getVariable("UserStatus")->setDescription("User defined status string.");
   getVariable("UserStatus")->setHidden(true);

   addVariable(new Variable("RunNumber",Variable::Status));
   getVariable("RunNumber")->setDescription("Run Number Since Start");
   getVariable("RunNumber")->setInt(0);

#ifdef MAKE_SW_VERSION
   addVariable(new Variable("SwVersion",Variable::Status));
   getVariable("SwVersion")->setDescription("Software version");
   getVariable("SwVersion")->set(MAKE_SW_VERSION);
#endif

   getVariable("Enabled")->setHidden(true);
}

// Deconstructor
System::~System ( ) {
   xmlCleanupParser();
   xmlMemoryDump();
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
   while ( swRunEnable_ && (runTotal < swRunCount_ || swRunCount_ == 0 )) {

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
      if ( swRunCount_ == 0 ) getVariable("RunProgress")->setInt(0);
      else getVariable("RunProgress")->setInt((uint)(((double)runTotal/(double)swRunCount_)*100.0));
   }

   if ( debug_ ) {
      cout << "System::runThread -> Name: " << name_ 
           << ", Run Stopped, RunTotal = " << dec << runTotal << endl;
   }

   sleep(1);

   // Set run
   if ( swRunCount_ == 0 ) getVariable("RunProgress")->setInt(100);
   else getVariable("RunProgress")->setInt((uint)(((double)runTotal/(double)swRunCount_)*100.0));
   getVariable("RunState")->set(swRunRetState_);
   swRunning_ = false;
}

// Start Run
void System::setRunState(string state) {
   stringstream err;
   stringstream tmp;
   uint         toCount;
   uint         runNumber;

   // Stopped state is requested
   if ( state == "Stopped" ) {

      if ( swRunEnable_ ) {
         swRunEnable_ = false;
         pthread_join(swRunThread_,NULL);
      }

      allStatusReq_ = true;
      addRunStop();
   }

   // Running state is requested
   else if ( state == "Running" && !swRunning_ ) {

      // Set run command here when re-implemented
      //device->setRuncommand("command");

      // Increment run number
      runNumber = getVariable("RunNumber")->getInt() + 1;
      getVariable("RunNumber")->setInt(runNumber);
      addRunStart();

      swRunRetState_ = get("RunState");
      swRunEnable_   = true;
      getVariable("RunState")->set("Running");

      // Setup run parameters
      swRunCount_ = getInt("RunCount");
      if      ( get("RunRate") == "2000Hz") swRunPeriod_ =     500;
      else if ( get("RunRate") == "1000Hz") swRunPeriod_ =    1000;
      else if ( get("RunRate") == "120Hz") swRunPeriod_ =    8333;
      else if ( get("RunRate") == "100Hz") swRunPeriod_ =   10000;
      else if ( get("RunRate") ==  "10Hz") swRunPeriod_ =  100000;
      else if ( get("RunRate") ==   "1Hz") swRunPeriod_ = 1000000;
      else swRunPeriod_ = 1000000;

      // Start thread
      if ( swRunCount_ == 0 || pthread_create(&swRunThread_,NULL,swRunStatic,this) ) {
         err << "System::startRun -> Failed to create runThread" << endl;
         if ( debug_ ) cout << err.str();
         getVariable("RunState")->set(swRunRetState_);
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
            getVariable("RunState")->set(swRunRetState_);
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
   time_t          ctme;

   clock_gettime(CLOCK_REALTIME,&stme);

   // Read defaults file
   if ( name == "SetDefaults" ) {
      parseXmlFile(defaults_);
      //softReset();
   }

   // Read and parse xml file
   else if ( name == "ReadXmlFile" ) {
      parseXmlFile(arg);
      //softReset();
   }

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
      os << "<system>" << endl << configString(true,true) << "</system>" << endl;
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
      os << "<system>" << endl << statusString(true,true) << "</system>" << endl;
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
      os << "<system>" << endl << structureString(true,true) << "</system>" << endl;
      os.close();
   }

   // Open data file
   else if ( name == "OpenDataFile" ) {
      command("CloseDataFile","");
      commLink_->openDataFile(getVariable("DataFile")->get(),getVariable("DataCompress")->getInt());
      commLink_->addConfig(configString(true,false));
      readStatus();
      commLink_->addStatus(statusString(true,false));
      getVariable("DataOpen")->set("True");
   }

   // Close data file
   else if ( name == "CloseDataFile" ) {
      if ( get("DataOpen") == "True" ) {
         readStatus();
         commLink_->addStatus(statusString(true,false));
         commLink_->closeDataFile();
         getVariable("DataOpen")->set("False");
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

   time(&ctme);
   if ( get("DebugCmdTime") == "True" ) {
      cout << "System::command -> Command " << name << " time results: " << endl
           << "   Start Time: " << dec << stme.tv_sec << "." << stme.tv_nsec << endl
           << "     End Time: " << dec << etme.tv_sec << "." << etme.tv_nsec << " - " << ctime(&ctme);
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
   uint         num;
   string       id;
 
   // Generate a random number for xml string name;
   num = rand(); 
   id = num;

   stat = "";
   configUpdate = false;
   try {

      // Parse string
      doc = xmlReadMemory(xml.c_str(), strlen(xml.c_str()), id.c_str(), NULL, 0);
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
      errorFlag_ = true;
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
   errorFlag_ = false;
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
string System::poll (ControlCmdMemory *cmem) {
   uint         curr;
   uint         rate;
   stringstream msg;
   time_t       currTime;
   bool         send;
   string       stateIn;

   time(&currTime);

   // Detect run stop
   if ( swRunEnable_ && !swRunning_ ) {
      swRunEnable_ = false;
      pthread_join(swRunThread_,NULL);
      addRunStop();
      allStatusReq_ = true;
      if ( swRunError_ != "" ) {
         errorBuffer_.append("<error>");
         errorBuffer_.append(swRunError_); 
         errorBuffer_.append("</error>\n");
         errorFlag_ = true;
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
      errorFlag_ = true;
      configureMsg_ = "A System Error Has Occured!\n";
      configureMsg_.append("Please HardReset and then configure!\n");
   }

   // Update state message
   msg.str("");
   msg << configureMsg_;
   msg << "System is is in run state '" << get("RunState") << "'" << endl;
   msg << stateIn;
   getVariable("SystemState")->set(msg.str());

   // Once a second updates
   if ( currTime != lastTime_ ) {
      lastTime_ = currTime;

      // Add timestamp to data file if running
      if ( swRunning_ || hwRunning_ ) addRunTime();

      // File counters
      getVariable("RegRxCount")->setInt(commLink_->regRxCount());
      getVariable("TimeoutCount")->setInt(commLink_->timeoutCount());
      getVariable("ErrorCount")->setInt(commLink_->errorCount());
      getVariable("UnexpectedCount")->setInt(commLink_->unexpectedCount());

      curr = commLink_->dataFileCount();
      if ( curr < lastFileCount_ ) rate = 0;
      else rate = curr - lastFileCount_;
      lastFileCount_ = curr;
      msg.str("");
      msg << dec << curr << " - " << dec << rate << " Hz";
      getVariable("DataFileCount")->set(msg.str());
   
      curr = commLink_->dataRxCount();
      if ( curr < lastDataCount_ ) rate = 0;
      else rate = curr - lastDataCount_;
      lastDataCount_ = curr;
      msg.str("");
      msg << dec << curr << " - " << dec << rate << " Hz";
      getVariable("DataRxCount")->set(msg.str());

      // Top status once a second
      topStatusReq_ = true;
   }

   // Generate outgoing message
   send = false;
   msg.str("");
   msg << "<system>" << endl;
   if ( errorBuffer_ != "" ) { msg << errorBuffer_; send = true; }
   if ( topStatusReq_ || allStatusReq_ ) { msg << statusString(false,false); send=true; }
   if ( allConfigReq_ ) { msg << configString(false,false); send=true; }
   msg << "</system>" << endl;

   // Do we add configuration updates to file?
   if ( allConfigReq_ ) commLink_->addConfig(configString(true,false));
   if ( allStatusReq_ || allConfigReq_ ) commLink_->addStatus(statusString(true,false));

   // Update shared memory
   if ( cmem != NULL ) {
      if ( errorBuffer_ != "" ) strcpy(controlErrorBuffer(cmem),errorBuffer_.c_str());
      if ( topStatusReq_ || allStatusReq_ ) strcpy(controlXmlStatusBuffer(cmem), statusString(true,false).c_str());
      if ( allConfigReq_ ) strcpy(controlXmlConfigBuffer(cmem), configString(true,false).c_str());
      if ( topStatusReq_ || allStatusReq_ || allConfigReq_ ) {
         strcpy(controlStatBuffer(cmem),get("SystemState").c_str());
         strcpy(controlUserBuffer(cmem),get("UserStatus").c_str());
      }
   }

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
string System::statusString(bool hidden, bool indent) {
   stringstream tmp;
   tmp.str("");
   if ( indent ) tmp << "   ";
   tmp << "<status>" << endl;
   tmp << getXmlStatus(true,hidden,((indent)?2:0));
   if ( indent ) tmp << "   ";
   tmp << "</status>" << endl;
   return(tmp.str());
}

// Return config string
string System::configString(bool hidden, bool indent) {
   stringstream tmp;
   tmp.str("");
   if ( indent ) tmp << "   ";
   tmp << "<config>" << endl;
   tmp << getXmlConfig(true,true,hidden,((indent)?2:0));  // Common
   tmp << getXmlConfig(true,false,hidden,((indent)?2:0)); // Per-Instance
   if ( indent ) tmp << "   ";
   tmp << "</config>" << endl;
   return(tmp.str());
}

// Return structure string
string System::structureString (bool hidden, bool indent) {
   stringstream tmp;
   tmp.str("");
   if ( indent ) tmp << "   ";
   tmp << "<structure>" << endl;
   tmp << getXmlStructure(true,true,hidden,((indent)?2:0));  // General
   tmp << getXmlStructure(true,false,hidden,((indent)?2:0)); // Per-Instance
   if ( indent ) tmp << "   ";
   tmp << "</structure>" << endl;
   return(tmp.str());
}

// Method to write configuration registers
void System::writeConfig ( bool force ) {
   
   // Update debug
   setDebug(getVariable("DebugEnable")->getInt());
   commLink_->setDebug(getVariable("DebugEnable")->getInt());

   Device::writeConfig(force);
}

// Set debug flag
void System::setDebug( bool enable ) {
   if ( enable && !debug_ ) cout << "System::setDebug -> Name: " << name_ << " Enabling debug messages." << endl;
   if ( debug_ && !enable ) cout << "System::setDebug -> Name: " << name_ << " Disabling debug messages." << endl;

   getVariable("DebugEnable")->setInt(enable);
   Device::setDebug(enable);
}

// Generate timestamp from passed time value
string System::genTime( time_t tme ) {
   char   tstr[200];
   struct tm *timeinfo;
   string ret;

   timeinfo = localtime(&tme);

   strftime(tstr,200,"%Y_%m_%d_%H_%M_%S",timeinfo);
   ret = tstr;
   return(ret); 
}

void System::addRunStart() {
   stringstream    xml;

   xml.str("");
   xml << "<runStart>" << endl;
   xml << "<runNumber>" << getVariable("RunNumber")->getInt() << "</runNumber>" << endl;
   xml << "<timestamp>" << genTime(time(0)) << "</timestamp>" << endl;
   xml << "<unixtime>" << dec << time(0) << "</unixtime>" << endl;
   xml << "<user>" << getlogin() << "</user>" << endl;
   xml << "</runStart>" << endl;
   commLink_->addRunStart(xml.str());
}

void System::addRunStop() {
   stringstream    xml;

   xml.str("");
   xml << "<runStop>" << endl;
   xml << "<runNumber>" << getVariable("RunNumber")->getInt() << "</runNumber>" << endl;
   xml << "<timestamp>" << genTime(time(0)) << "</timestamp>" << endl;
   xml << "<unixtime>" << dec << time(0) << "</unixtime>" << endl;
   xml << "<user>" << getlogin() << "</user>" << endl;
   xml << "</runStop>" << endl;
   commLink_->addRunStop(xml.str());
}

void System::addRunTime() {
   stringstream    xml;

   xml.str("");
   xml << "<runTime>" << endl;
   xml << "<runNumber>" << getVariable("RunNumber")->getInt() << "</runNumber>" << endl;
   xml << "<timestamp>" << genTime(time(0)) << "</timestamp>" << endl;
   xml << "<unixtime>" << dec << time(0) << "</unixtime>" << endl;
   xml << "<user>" << getlogin() << "</user>" << endl;
   xml << "</runTime>" << endl;
   commLink_->addRunTime(xml.str());
}

