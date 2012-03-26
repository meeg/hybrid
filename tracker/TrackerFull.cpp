//-----------------------------------------------------------------------------
// File          : TrackerFull.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon TrackerFull
//-----------------------------------------------------------------------------
// Description :
// Control FPGA container
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#include <TrackerFull.h>
#include <CntrlFpga.h>
#include <TisFpga.h>
#include <Register.h>
#include <Variable.h>
#include <Command.h>
#include <CommLink.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;

// Constructor
TrackerFull::TrackerFull (CommLink *commLink) : System("TrackerFull",commLink) {

   // Description
   desc_ = "TrackerFull Control";
   defaults_ = "config/coda_defaults.xml";
   
   // Data mask, lane 0, vc 0
   commLink_->setDataMask(0x11);

   // Add sub-devices
   addDevice(new CntrlFpga(0, 0,this));
   addDevice(new CntrlFpga(1, 1,this));
   addDevice(new CntrlFpga(2, 2,this));
   addDevice(new CntrlFpga(3, 3,this));
   addDevice(new CntrlFpga(4, 4,this));
   addDevice(new CntrlFpga(5, 5,this));
   addDevice(new CntrlFpga(6, 6,this));
   addDevice(new TisFpga(7, 0,this));

   addVariable(new Variable("CodaState", Variable::Status));
   variables_["CodaState"]->setDescription("Coda State");

   addVariable(new Variable("TholdFile", Variable::Configuration));
   variables_["TholdFile"]->setDescription("Threshold File");

   addVariable(new Variable("TholdId", Variable::Status));
   variables_["TholdId"]->setDescription("Threshold ID Value From File");

   addVariable(new Variable("TholdFile", Variable::Status));
   variables_["TholdFile"]->setDescription("Threshold File");

   addVariable(new Variable("TempPollPer", Variable::Configuration));
   variables_["TempPollPer"]->setDescription("Temperature polling period for epics in seconds");
   variables_["TempPollPer"]->setRange(0,3600);
   variables_["TempPollPer"]->setComp(0,1,0,"S");

   addCommand(new Command("CodaPrestart"));
   commands_["CodaPrestart"]->setDescription("Coda Prestart");

   addCommand(new Command("CodaGo"));
   commands_["CodaGo"]->setDescription("Coda Go");

   addCommand(new Command("CodaGoInt"));
   commands_["CodaGoInt"]->setDescription("Coda Go Int Triggers");

   addCommand(new Command("CodaPause"));
   commands_["CodaPause"]->setDescription("Coda Pause");

   addCommand(new Command("CodaEnd"));
   commands_["CodaEnd"]->setDescription("Coda Done");

   addCommand(new Command("DumpThreshold"));
   commands_["DumpThreshold"]->setDescription("Dump Threshold To File");

   epicsDataOpenAndMap(&tempMem_);       

   // Init threshold data
   memset(&thold_,0,sizeof(Threshold));
}

// Deconstructor
TrackerFull::~TrackerFull ( ) { }

// Method to process a command
void TrackerFull::command ( string name, string arg ) {
   string       command;
   stringstream tmp;
   stringstream dstr;
   long         tme;
   struct tm    *tm_data;
   uint         fpga,hyb,apv,chan;
   ofstream     os;
   string       fname;

   if ( name == "CodaPrestart" ) {
      cout << "TrackerFull::command -> Executing CodaPrestart" << endl;
      //System::command("HardReset","");
      //System::command("SetDefaults","");
      command  = "<system><config>";
      command += "<cntrlFpga>";
      command += "<ApvTrigSource>None</ApvTrigSource>";
      command += "</cntrlFpga>";
      command += "<tisFpga>";
      command += "<TrigEnable>False</TrigEnable>";
      command += "</tisFpga>";
      command += "</config></system>";
      parseXmlString(command);
      System::command("SoftReset","");
      variables_["CodaState"]->set("Prestart");

      time(&tme);
      tm_data = localtime(&tme);
      dstr.str("");
      dstr << "./logs/";
      dstr << dec << (tm_data->tm_year + 1900) << "_";
      dstr << dec << setw(2) << setfill('0') << (tm_data->tm_mon+1) << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_mday    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_hour    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_min     << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_sec;

      tmp.str("");
      tmp << dstr.str() << "_CodaPrestart_Config.xml";
      System::command("WriteConfigXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaPrestart_Status.xml";
      System::command("WriteStatusXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaPrestart_Threshold.xml";
      this->command("DumpThreshold",tmp.str());
   }

   else if ( name == "CodaGo" ) {
      cout << "TrackerFull::command -> Executing CodaGo" << endl;
      command  = "<system><config>";
      command += "<cntrlFpga>";
      command += "<ApvTrigSource>External</ApvTrigSource>";
      command += "</cntrlFpga>";
      command += "<tisFpga>";
      command += "<TrigEnable>True</TrigEnable>";
      command += "</tisFpga>";
      command += "</config></system>";
      parseXmlString(command);
      variables_["CodaState"]->set("Go");

      time(&tme);
      tm_data = localtime(&tme);
      dstr.str("");
      dstr << "./logs/";
      dstr << dec << (tm_data->tm_year + 1900) << "_";
      dstr << dec << setw(2) << setfill('0') << (tm_data->tm_mon+1) << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_mday    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_hour    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_min     << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_sec;

      tmp.str("");
      tmp << dstr.str() << "_CodaGo_Config.xml";
      System::command("WriteConfigXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaGo_Status.xml";
      System::command("WriteStatusXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaGo_Threshold.xml";
      this->command("DumpThreshold",tmp.str());
   }

   else if ( name == "CodaGoInt" ) {
      cout << "TrackerFull::command -> Executing CodaGoInt" << endl;
      command  = "<system><config>";
      command += "<cntrlFpga>";
      command += "<ApvTrigSource>External</ApvTrigSource>";
      command += "</cntrlFpga>";
      command += "<tisFpga>";
      command += "<TrigEnable>True</TrigEnable>";
      command += "</tisFpga>";
      command += "</config></system>";
      parseXmlString(command);
      variables_["CodaState"]->set("Go Internal");

      time(&tme);
      tm_data = localtime(&tme);
      dstr.str("");
      dstr << "./logs/";
      dstr << dec << (tm_data->tm_year + 1900) << "_";
      dstr << dec << setw(2) << setfill('0') << (tm_data->tm_mon+1) << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_mday    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_hour    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_min     << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_sec;

      tmp.str("");
      tmp << dstr.str() << "_CodaGoInt_Config.xml";
      System::command("WriteConfigXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaGoInt_Status.xml";
      System::command("WriteStatusXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaGoInt_Threshold.xml";
      this->command("DumpThreshold",tmp.str());

      device("tisFpga",0)->command("IntTrigStart","");
   }

   else if ( name == "CodaPause" ) {
      cout << "TrackerFull::command -> Executing CodaPause" << endl;
      sleep(1);
      command  = "<system><config>";
      command += "<cntrlFpga>";
      command += "<ApvTrigSource>None</ApvTrigSource>";
      command += "</cntrlFpga>";
      command += "<tisFpga>";
      command += "<TrigEnable>False</TrigEnable>";
      command += "</tisFpga>";
      command += "</config></system>";
      parseXmlString(command);
      variables_["CodaState"]->set("Pause");
      sleep(1);

      time(&tme);
      tm_data = localtime(&tme);
      dstr.str("");
      dstr << "./logs/";
      dstr << dec << (tm_data->tm_year + 1900) << "_";
      dstr << dec << setw(2) << setfill('0') << (tm_data->tm_mon+1) << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_mday    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_hour    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_min     << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_sec;

      tmp.str("");
      tmp << dstr.str() << "_CodaPause_Config.xml";
      System::command("WriteConfigXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaPause_Status.xml";
      System::command("WriteStatusXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaPause_Threshold.xml";
      this->command("DumpThreshold",tmp.str());
   }

   else if ( name == "CodaEnd" ) {
      cout << "TrackerFull::command -> Executing CodaEnd" << endl;
      sleep(1);
      command  = "<system><config>";
      command += "<cntrlFpga>";
      command += "<ApvTrigSource>None</ApvTrigSource>";
      command += "</cntrlFpga>";
      command += "<tisFpga>";
      command += "<TrigEnable>False</TrigEnable>";
      command += "</tisFpga>";
      command += "</config></system>";
      parseXmlString(command);
      variables_["CodaState"]->set("End");
      sleep(1);

      time(&tme);
      tm_data = localtime(&tme);
      dstr.str("");
      dstr << "./logs/";
      dstr << dec << (tm_data->tm_year + 1900) << "_";
      dstr << dec << setw(2) << setfill('0') << (tm_data->tm_mon+1) << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_mday    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_hour    << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_min     << "_";
      dstr << dec << setw(2) << setfill('0') << tm_data->tm_sec;

      tmp.str("");
      tmp << dstr.str() << "_CodaEnd_Config.xml";
      System::command("WriteConfigXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaEnd_Status.xml";
      System::command("WriteStatusXml",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaEnd_Threshold.xml";
      this->command("DumpThreshold",tmp.str());
   }

   else if ( name == "DumpThreshold" ) {
      if ( arg == "" ) fname = "thresholdDump.txt";
      else fname = arg;

      os.open(fname.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "TrackerFull::command -> Error opening threshold file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }

      os << "Id: " << thold_.thresholdId << endl;
      os << "Hybrid APV Channel Fpga_0     Fpga_1     Fpga_2     Fpga_3     Fpga_4     Fpga_5     Fpga_6" << endl;

      for(hyb=0; hyb <3; hyb++) {
         for(apv=0; apv <5; apv++) {
            for(chan=0; chan <128; chan++) {
               os << dec << hyb << "      " << dec << apv << "   " << dec << setw(3) << setfill('0') << chan << "     ";

               for(fpga=0; fpga < 7; fpga++) {
                  os << "0x" << setw(8) << setfill('0') << hex << thold_.threshData[fpga][hyb][apv][chan] << " ";
               }
               os << endl;
            }
         }
      }
      os.close();
   }
   else System::command(name,arg);
}

// Method to set run state
void TrackerFull::setRunState ( string state ) {
   if ( !swRunning_ ) device("tisFpga",0)->setRunCommand("TisSWTrig");
   System::setRunState(state);
}

// Get extracted temp value
double TrackerFull::getTemp(uint fpga, uint index) {
   string       valName;
   string       valIn;
   double       valOut;
   char         buffa[50];
   char         buffb[50];

   switch(index) {
      case  0: valName = "Temp_0_0"; break; 
      case  1: valName = "Temp_0_1"; break; 
      case  2: valName = "Temp_0_2"; break; 
      case  3: valName = "Temp_0_3"; break; 
      case  4: valName = "Temp_1_0"; break; 
      case  5: valName = "Temp_1_1"; break; 
      case  6: valName = "Temp_1_2"; break; 
      case  7: valName = "Temp_1_3"; break; 
      case  8: valName = "Temp_2_0"; break; 
      case  9: valName = "Temp_2_1"; break; 
      case 10: valName = "Temp_2_2"; break; 
      case 11: valName = "Temp_2_3"; break; 
      default: valName = ""; break;
   }
 
   valIn = device("cntrlFpga",fpga)->get(valName);
   sscanf(valIn.c_str(),"%lf %s %s",&valOut,buffa,buffb);

   return(valOut);
}

//! Return local state, specific to each implementation
string TrackerFull::localState() {
   stringstream loc;
   uint         apv;
   uint         bit;
   uint         hyb;
   uint         fpga;
   uint         tmp;
   uint         apvCnt;
   uint         syncCnt;
   uint         reg;
   bool         fifoErr;
   bool         latErr;
   uint         fpgaMask;
   uint         eventSize;
   uint         errorFlag;
   time_t       currTime;
   stringstream stat;
   CntrlFpga    *f;

   apvCnt    = 0;
   syncCnt   = 0;
   fifoErr   = false;
   latErr    = false;
   errorFlag = errorFlag_;
   fpgaMask  = 0x80; // TI Always Enabled
   loc.str("");

   // Check hybrid status
   for ( fpga = 0; fpga < 7; fpga++ ) { 
      if ( device("cntrlFpga",fpga)->get("enabled") == "True" ) {
         fpgaMask |= (1 << fpga);
      
         // Get sync register
         reg = device("cntrlFpga",fpga)->getInt("ApvSyncDetect");

         for ( hyb = 0; hyb < 3; hyb++ ) { 
            if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->get("enabled") == "True" ) {

               // Check APV status registers
               for ( apv = 0; apv < 5; apv++ ) {
                  if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->get("enabled") == "True" ) {
                     if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->getInt("FifoError") != 0 ) {
                        fifoErr = true;
                        errorFlag = 1;
                     }
                     if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->getInt("LatencyError") != 0 ) {
                        latErr = true;
                        errorFlag = 1;
                     }

                     apvCnt++;
                  }

                  // Count synced apvs
                  bit = (hyb * 5) + apv;
                  if (((reg >> bit) & 0x1 ) != 0) syncCnt++;
               }
            }
         }
      }
   }


   // Get Event Size
   eventSize = device("tisFpga",0)->getInt("EventSize") + 1;

   // Update status message
   if ( fifoErr ) loc << "APV FIFO Error.\n";
   if ( latErr  ) loc << "APV Latency Error.\n";
   loc << dec << syncCnt << " Out Of " << dec << apvCnt << " Apvs Are Synced!\n";
   if ( (!fifoErr) && (!latErr) ) loc << "System Ready To Take Data.\n";      
   else loc << "Soft Reset Required!\n";

   // Add coda state
   if ( variables_["CodaState"]->get() != "" ) 
      loc << "Coda state is '" << variables_["CodaState"]->get() << "'\n";

   // Update user state variable
   stat.str("");
   stat << dec << fpgaMask << " " << dec << eventSize << " " << dec << errorFlag;
   variables_["UserStatus"]->set(stat.str());

   // Polling is enabled
   time(&currTime);
   if ( getInt("TempPollPer") > 0 ) {

      // Check if it is time for an update
      if ( (currTime - lastTemp_) > getInt("TempPollPer") ) {

         // Poll each fpga
         for ( fpga = 0; fpga < 7; fpga++ ) { 
            if ( device("cntrlFpga",fpga)->get("enabled") == "True" ) {

               // Read current values
               try {
                  f = (CntrlFpga *)(device("cntrlFpga",fpga));
                  f->readTemps();
               } catch (string error) {
                  for (tmp =0; tmp < 12; tmp++) epicsDataSet(tempMem_,fpga*12+tmp,0);
                  cout << "TrackerFull::localState -> Got error reading temperatures "
                       << "from fpga " << dec << fpga << ". Ignoring" << endl;
               }
               
               // Get 12 temperatures from each fpga
               for (tmp =0; tmp < 12; tmp++) epicsDataSet(tempMem_,fpga*12+tmp,getTemp(fpga,tmp));
            }
         }

         // Mark shared memory as updated
         epicsDataSetReady(tempMem_);
         lastTemp_ = currTime;
      }
   }
   else lastTemp_ = currTime;

   return(loc.str());
}

//! Method to perform soft reset
void TrackerFull::softReset ( ) {
   uint x;

   System::softReset();

   for (x=0; x < 7; x++) device("cntrlFpga",x)->command("TrigCntRst","");
   device("tisFpga",0)->command("TrigCntRst","");
   
   for (x=0; x < 7; x++) device("cntrlFpga",x)->command("ApvClkRst","");
   sleep(1);
   for (x=0; x < 7; x++) device("cntrlFpga",x)->command("Apv25Reset","");
   sleep(1);
   for (x=0; x < 7; x++) device("cntrlFpga",x)->command("Apv25Reset","");
   sleep(1);
}

//! Method to perform hard reset
void TrackerFull::hardReset ( ) {
   stringstream error;
   bool gotVer = false;
   uint count = 0;
   uint x;

   variables_["CodaState"]->set("");

   for (x=0; x < 7; x++) {
      device("cntrlFpga",x)->device("hybrid",0)->set("enabled","False");
      device("cntrlFpga",x)->device("hybrid",1)->set("enabled","False");
      device("cntrlFpga",x)->device("hybrid",2)->set("enabled","False");
   }

   System::hardReset();

   //for (x=0; x < 7; x++) device("cntrlFpga",x)->command("MasterReset","");

   do {
      sleep(1);
      try { 
         gotVer = true;
         for (x=0; x < 7; x++) device("cntrlFpga",x)->readSingle("Version");
      } catch ( string err ) { 
         if ( count > 5 ) {
            gotVer = true;
            error.str("");
            error << "TrackerFull::hardReset -> Error reading version regsiter from fpga " << x;
            cout << error.str() << endl;
            throw(string(error.str()));
         }
         else {
            count++;
            gotVer = false;
         }
      }
   } while ( !gotVer );
   for (x=0; x < 7; x++) device("cntrlFpga",x)->command("Apv25HardReset","");
   usleep(100);
}

// Method to write configuration registers
void TrackerFull::writeConfig ( bool force ) {
   CntrlFpga    *f;
   uint         x;
   int          fd;

   REGISTER_LOCK

   // Set threshold pointer to FPGAs
   for (x=0; x < 7; x++) {
      f = (CntrlFpga *)(device("cntrlFpga",x));
      f->setThreshold(&thold_);
   }

   // Attempt to open file
   if ( get("TholdFile") != "" ) {
      if ( (fd = open(get("TholdFile").c_str(),O_RDONLY)) < 0 ) {
         cout << "TrackerFull::writeConfig -> Error opening threshold file: " << get("TholdFile") << endl;
         fd = -1;
      }
   } else fd = -1;

   // File is valid
   if ( fd > 0 ) {

      // Read in threshold data
      read(fd,&thold_,sizeof(Threshold));

      // Set ID variable
      variables_["TholdId"]->set(thold_.thresholdId);

   } else {
      set("TholdId","None");
      memset(&thold_,0,sizeof(Threshold));
   }

   // Sub devices
   Device::writeConfig(force);
   REGISTER_UNLOCK
}

