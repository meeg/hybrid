//-----------------------------------------------------------------------------
// File          : Tracker.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : Heavy Photon Tracker
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
#include <Tracker.h>
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
Tracker::Tracker (CommLink *commLink, string defaults, uint fpgaCount, bool tisEnable) : System("Tracker",commLink) {
   uint x;

   // Description
   desc_      = "Tracker Control";
   defaults_  = defaults;
   fpgaCount_ = fpgaCount;
   tisEnable_ = tisEnable;
   
   // Data mask, lane 0, vc 0
   commLink_->setDataMask(0x11);

   // Add sub-devices
   for (x=0; x < fpgaCount; x++) addDevice(new CntrlFpga(x, x,this));
   if ( tisEnable_ ) addDevice(new TisFpga(7, 0,this));

   addVariable(new Variable("CodaState", Variable::Status));
   variables_["CodaState"]->setDescription("Coda State");

   addVariable(new Variable("TholdFile", Variable::Configuration));
   variables_["TholdFile"]->setDescription("Threshold File");

   addVariable(new Variable("TholdId", Variable::Status));
   variables_["TholdId"]->setDescription("Threshold ID Value From File");

   addVariable(new Variable("FilterFile", Variable::Configuration));
   variables_["FilterFile"]->setDescription("Filter File");

   addVariable(new Variable("FilterId", Variable::Status));
   variables_["FilterId"]->setDescription("Filter ID Value From File");

   addVariable(new Variable("TempPollPer", Variable::Configuration));
   variables_["TempPollPer"]->setDescription("Temperature polling period for epics in seconds");
   variables_["TempPollPer"]->setRange(0,3600);
   variables_["TempPollPer"]->setComp(0,1,0,"S");

   addVariable(new Variable("IntTrigEn", Variable::Configuration));
   variables_["IntTrigEn"]->setTrueFalse();

   addVariable(new Variable("LiveDisplay", Variable::Configuration));
   variables_["LiveDisplay"]->setTrueFalse();

   addVariable(new Variable("IgnoreErrors", Variable::Configuration));
   variables_["IgnoreErrors"]->setTrueFalse();

   addVariable(new Variable("TrigCount", Variable::Status));
   addVariable(new Variable("TrigDropCount", Variable::Status));

   addVariable(new Variable("TrigPollEn", Variable::Configuration));
   variables_["TrigPollEn"]->setTrueFalse();

   addCommand(new Command("CodaPrestart"));
   commands_["CodaPrestart"]->setDescription("Coda Prestart");

   addCommand(new Command("CodaGo"));
   commands_["CodaGo"]->setDescription("Coda Go");

   addCommand(new Command("CodaPause"));
   commands_["CodaPause"]->setDescription("Coda Pause");

   addCommand(new Command("CodaEnd"));
   commands_["CodaEnd"]->setDescription("Coda Done");

   addCommand(new Command("IntTrigStart"));
   commands_["IntTrigStart"]->setDescription("Start Internal Trigger");

   addCommand(new Command("DumpThreshold"));
   commands_["DumpThreshold"]->setDescription("Dump Threshold To File");

   addCommand(new Command("DumpFilter"));
   commands_["DumpFilter"]->setDescription("Dump Filter To File");

   epicsDataOpenAndMap(&tempMem_);       

   // Init threshold data
   memset(&filt_,0,sizeof(Filter));

   softResetPressed_ = false;
}

// Deconstructor
Tracker::~Tracker ( ) { }

// Method to process a command
void Tracker::command ( string name, string arg ) {
   string       command;
   stringstream tmp;
   stringstream dstr;
   long         tme;
   struct tm    *tm_data;
   uint         fpga,hyb,apv,chan;
   ofstream     os;
   string       fname;
   uint         x;

   if ( name == "CodaPrestart" ) {
      cout << "Tracker::command -> Executing CodaPrestart" << endl;
      if ( tisEnable_ ) {
         command  = "<system><config>";
         command += "<tisFpga>";
         command += "<TrigEnable>False</TrigEnable>";
         command += "</tisFpga>";
         command += "</config></system>";
         parseXmlString(command);
      }

      for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("TrigCntRst","");
      if ( tisEnable_ ) {
         device("tisFpga",0)->command("TrigCntRst","");
         device("tisFpga",0)->command("DropCountReset","");
      }

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
      tmp << dstr.str() << "_CodaPrestart_Threshold.txt";
      this->command("DumpThreshold",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaPrestart_Filter.txt";
      this->command("DumpFilter",tmp.str());
   }

   else if ( name == "CodaGo" ) {
      cout << "Tracker::command -> Executing CodaGo" << endl;
      if ( tisEnable_ ) {
         command  = "<system><config>";
         command += "<tisFpga>";
         command += "<TrigEnable>True</TrigEnable>";
         command += "</tisFpga>";
         command += "</config></system>";
         parseXmlString(command);
      }

      for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("TrigCntRst","");
      if ( tisEnable_ ) {
         device("tisFpga",0)->command("TrigCntRst","");
         device("tisFpga",0)->command("DropCountReset","");
      }

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
      tmp << dstr.str() << "_CodaGo_Threshold.txt";
      this->command("DumpThreshold",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaGo_Filter.txt";
      this->command("DumpFilter",tmp.str());

      if ( getInt("IntTrigEn") == 1 ) device("tisFpga")->command("IntTrigStart","");
   }

   else if ( name == "CodaPause" ) {
      cout << "Tracker::command -> Executing CodaPause" << endl;
      sleep(1);
      if ( tisEnable_ ) {
         command  = "<system><config>";
         command += "<tisFpga>";
         command += "<TrigEnable>False</TrigEnable>";
         command += "</tisFpga>";
         command += "</config></system>";
         parseXmlString(command);
      }
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
      tmp << dstr.str() << "_CodaPause_Threshold.txt";
      this->command("DumpThreshold",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaPause_Filter.txt";
      this->command("DumpFilter",tmp.str());
   }

   else if ( name == "CodaEnd" ) {
      cout << "Tracker::command -> Executing CodaEnd" << endl;
      sleep(1);
      if ( tisEnable_ ) {
         command  = "<system><config>";
         command += "<tisFpga>";
         command += "<TrigEnable>False</TrigEnable>";
         command += "</tisFpga>";
         command += "</config></system>";
         parseXmlString(command);
      }
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
      tmp << dstr.str() << "_CodaEnd_Threshold.txt";
      this->command("DumpThreshold",tmp.str());

      tmp.str("");
      tmp << dstr.str() << "_CodaEnd_Filter.txt";
      this->command("DumpFilter",tmp.str());
   }

   else if ( name == "DumpThreshold" ) {
      if ( arg == "" ) fname = "thresholdDump.txt";
      else fname = arg;

      os.open(fname.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "Tracker::command -> Error opening threshold file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }

      os << "Id: " << thold_.thresholdId << endl;
      os << "Hybrid APV Channel Fpga_0     Fpga_1     Fpga_2     Fpga_3     Fpga_4     Fpga_5     Fpga_6" << endl;

      for(hyb=0; hyb <3; hyb++) {
         for(apv=0; apv <5; apv++) {
            for(chan=0; chan <128; chan++) {
               os << dec << hyb << "      " << dec << apv << "   " << dec << setw(3) << setfill('0') << chan << "     ";

               for(fpga=0; fpga < fpgaCount_; fpga++) {
                  os << "0x" << setw(8) << setfill('0') << hex << thold_.threshData[fpga][hyb][apv][chan] << " ";
               }
               os << endl;
            }
         }
      }
      os.close();
   }

   else if ( name == "DumpFilter" ) {
      if ( arg == "" ) fname = "filterDump.txt";
      else fname = arg;

      os.open(fname.c_str(),ios::out | ios::trunc);
      if ( ! os.is_open() ) {
         tmp.str("");
         tmp << "Tracker::command -> Error opening filter file for write: " << arg << endl;
         if ( debug_ ) cout << tmp.str();
         throw(tmp.str());
      }

      os << "Id: " << filt_.filterId << endl;
      os << "Hybrid APV Coef    Fpga_0     Fpga_1     Fpga_2     Fpga_3     Fpga_4     Fpga_5     Fpga_6" << endl;

      for(hyb=0; hyb <3; hyb++) {
         for(apv=0; apv <5; apv++) {
            for(chan=0; chan <10; chan++) {
               os << dec << hyb << "      " << dec << apv << "   " << dec << chan << "       ";

               for(fpga=0; fpga < fpgaCount_; fpga++) {
                  os << setw(6) << setfill('0') << hex << filt_.filterData[fpga][hyb][apv][chan] << " ";
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
void Tracker::setRunState ( string state ) {

   if ( get("CodaState") != "" ) {
      cout << "Tracker::setRunState -> Can't start manual run while connected to coda!!!!!" << endl;
      return;
   }

   if ( !swRunning_ ) {
      if ( tisEnable_ ) device("tisFpga",0)->setRunCommand("TisSWTrig");
      else device("cntrlFpga",0)->setRunCommand("ApvSWTrig");
   }
   System::setRunState(state);
}

// Get extracted temp value
double Tracker::getTemp(uint fpga, uint index) {
   string       valName;
   string       valIn;
   double       valOut;
   char         buffa[50];
   char         buffb[50];

   if ( fpga >= fpgaCount_ ) return(0);

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
string Tracker::localState() {
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
   uint         userError;
   time_t       currTime;
   stringstream stat;
   CntrlFpga    *f;
   TisFpga      *t;

   apvCnt    = 0;
   syncCnt   = 0;
   fifoErr   = false;
   latErr    = false;
   errorFlag = errorFlag_;
   fpgaMask  = 0x80; // TI Always Enabled
   loc.str("");

   // Temperature Polling
   time(&currTime);
   if ( getInt("TempPollPer") > 0 ) {
   
      // Check if it is time for an update
      if ( (currTime - lastTemp_) > getInt("TempPollPer") ) {
         cout << "HeartBeat: " << ctime(&currTime);

         try {

            // Poll each fpga
            for ( fpga = 0; fpga < fpgaCount_; fpga++ ) { 
               if ( device("cntrlFpga",fpga)->get("Enabled") == "True" ) {

                  // Read current values
                  f = (CntrlFpga *)(device("cntrlFpga",fpga));
                  f->readTemps();
                  
                  // Get 12 temperatures from each fpga
                  for (tmp =0; tmp < 12; tmp++) epicsDataSet(tempMem_,fpga*12+tmp,getTemp(fpga,tmp));
               }
            }

            // Mark shared memory as updated, only if no errors
            epicsDataSetReady(tempMem_);

         } catch (string error) {
            for (tmp =0; tmp < 12; tmp++) epicsDataSet(tempMem_,fpga*12+tmp,0);
            cout << "Tracker::localState -> Got error reading temperatures "
                 << "from fpga " << dec << fpga << ". Ignoring" << endl;
         }
         lastTemp_ = currTime;
      }
   }
   else lastTemp_ = currTime;

   // Check hybrid status
   for ( fpga = 0; fpga < fpgaCount_; fpga++ ) { 
      if ( device("cntrlFpga",fpga)->get("Enabled") == "True" ) {
         fpgaMask |= (1 << fpga);
      
         // Get sync register
         reg = device("cntrlFpga",fpga)->getInt("ApvSyncDetect");

         for ( hyb = 0; hyb < 3; hyb++ ) { 
            if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->get("Enabled") == "True" ) {

               // Check APV status registers
               for ( apv = 0; apv < 5; apv++ ) {
                  if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->get("Enabled") == "True" ) {
                     if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->getInt("FifoError") != 0 ) {
                        loc << "Fpga " << dec << fpga << " Hybrid " << dec << hyb << " Apv " << apv << " Fifo Error" << endl;
                        fifoErr = true;
                        errorFlag = 1;
                     }
                     if ( device("cntrlFpga",fpga)->device("hybrid",hyb)->device("apv25",apv)->getInt("LatencyError") != 0 ) {
                        loc << "Fpga " << dec << fpga << " Hybrid " << dec << hyb << " Apv " << apv << " Latency Error" << endl;
                        latErr = true;
                        errorFlag = 1;
                     }

                     apvCnt++;

                     // Count synced apvs
                     bit = (hyb * 5) + apv;
                     if (((reg >> bit) & 0x1 ) != 0) syncCnt++;
                     else {
                        loc << "Fpga " << dec << fpga << " Hybrid " << dec << hyb << " Apv " << apv << " Not synced" << endl;
                     }
                  }
               }
            }
         }
      }
   }

   // Get Event Size
   if ( tisEnable_ ) eventSize = device("tisFpga",0)->getInt("EventSize") + 1;

   // Update status message
   if ( fifoErr ) loc << "APV FIFO Error.\n";
   if ( latErr  ) loc << "APV Latency Error.\n";
   loc << dec << syncCnt << " Out Of " << dec << apvCnt << " Apvs Are Synced!\n";
   if ( fifoErr || latErr ) loc << "Soft Reset Required!\n";

   // Add coda state
   if ( variables_["CodaState"]->get() != "" ) 
      loc << "Coda state is '" << variables_["CodaState"]->get() << "'\n";

   // Update user state variable
   if ( getInt("IgnoreErrors") ) userError = 0;
   else userError = errorFlag;
   stat.str("");
   stat << dec << fpgaMask << " " << dec << eventSize << " " << dec << userError << " " << dec << getInt("IntTrigEn") 
        << " " << dec << getInt("LiveDisplay") << endl;
   variables_["UserStatus"]->set(stat.str());

   // Read trigger counts
   if ( tisEnable_ && (variables_["TrigPollEn"]->getInt() && currTime != lastTrig_) ) {
      try {
         lastTrig_ = currTime;
         t = (TisFpga *)(device("tisFpga",0));
         t->readTrig();
   
         variables_["TrigCount"]->set(t->get("TrigCount"));
         variables_["TrigDropCount"]->set(t->get("TrigDropCount"));
      } catch ( string errror ) {
         cout << "Tracker::localState -> Error reading trigger counts" << endl;
      }
   }

   if ( softResetPressed_ ) {
      cout << "Status: " << ctime(&currTime) << endl;
      cout << loc.str() << endl;
      softResetPressed_ = false;
   }

   return(loc.str());
}

//! Method to perform soft reset
void Tracker::softReset ( ) {
   uint   x;

   System::softReset();

   for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("TrigCntRst","");
   if ( tisEnable_ ) {
      device("tisFpga",0)->command("TisClkReset","");
      device("tisFpga",0)->command("TrigCntRst","");
      device("tisFpga",0)->command("DropCountReset","");
   }
   
   for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("ApvClkRst","");
   sleep(1);
   for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("Apv25Reset","");
   sleep(1);
   for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("Apv25Reset","");
   sleep(1);

   softResetPressed_ = true;
}

//! Method to perform hard reset
void Tracker::hardReset ( ) {
   stringstream error;
   bool gotVer = false;
   uint count = 0;
   uint x;

   variables_["CodaState"]->set("");

   for (x=0; x < fpgaCount_; x++) {
      device("cntrlFpga",x)->device("hybrid",0)->set("Enabled","False");
      device("cntrlFpga",x)->device("hybrid",1)->set("Enabled","False");
      device("cntrlFpga",x)->device("hybrid",2)->set("Enabled","False");
   }

   System::hardReset();

   //for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("MasterReset","");

   do {
      sleep(1);
      try { 
         gotVer = true;
         for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->readSingle("Version");
      } catch ( string err ) { 
         if ( count > 5 ) {
            gotVer = true;
            error.str("");
            error << "Tracker::hardReset -> Error reading version regsiter from fpga " << x;
            cout << error.str() << endl;
            throw(string(error.str()));
         }
         else {
            count++;
            gotVer = false;
         }
      }
   } while ( !gotVer );
   for (x=0; x < fpgaCount_; x++) device("cntrlFpga",x)->command("Apv25HardReset","");

   if ( tisEnable_ ) device("tisFpga",0)->command("TisClkReset","");

   usleep(100);
}

// Method to write configuration registers
void Tracker::writeConfig ( bool force ) {
   CntrlFpga    *f;
   uint         x;
   stringstream tmp;
   ifstream     is;
   string       line;
   uint         fpga,hyb,apv,chan;

   REGISTER_LOCK

   // Set threshold pointer to FPGAs
   for (x=0; x < fpgaCount_; x++) {
      f = (CntrlFpga *)(device("cntrlFpga",x));
      f->setThreshold(&thold_);
      f->setFilter(&filt_);
   }

   // Get the file name and attemp to open it
   if( get("TholdFile") != "" && !thold_.openFile(get("TholdFile"))) {
	   cout << "Tracker::writeConfig -> Error opening threshold file: " << get("TholdFile") << endl;	     	
   } else if( get("TholdFile") == ""){
	thold_.loadDefaults();
   } else { 
	   // Set the ID variable
	   thold_.loadThresholdData();
   }

   // Time stamp the time that the thresholds were loaded
   variables_["TholdId"]->set(thold_.thresholdId);

   // Init filter data
   set("FilterId","None");
   for(fpga=0; fpga < fpgaCount_; fpga++) {
      for(hyb=0; hyb <3; hyb++) {
         for(apv=0; apv <5; apv++) {
            filt_.filterData[fpga][hyb][apv][0] = 1;
            for(chan=1; chan <10; chan++) {
               filt_.filterData[fpga][hyb][apv][chan] = 0;
            }
         }
      }
   }

   // Attempt to open file
   if ( get("FilterFile") != "" ) {
      is.open(get("FilterFile").c_str());
      if ( ! is.is_open() ) {
	      tmp.str("");
	      tmp << "Tracker::command -> Error opening filter file for read: " << get("FilterFile") << endl;
	      cout << tmp.str();
      }
      else {
	      tmp << "Tracker::command -> Reading from filter file: " << get("FilterFile") << endl;

         //read filter ID
         while (getline(is,line)) {
            if (line[0]!='#') break;
         }
         line.copy(filt_.filterId,filt_.IdLength);

         while (getline(is, line)) {
            if (line[0]=='#') continue;
            //printf("%s\n",line.c_str());
            istringstream iss(line);
            if (!(iss >> fpga >> hyb >> apv)) { break; } // error
            for (uint i = 0;i<filt_.CoefCount;i++)
               iss >> filt_.filterData[fpga][hyb][apv][i];
         }
         is.close();
      }
      variables_["FilterId"]->set(filt_.filterId);
   }

   // Sub devices
   Device::writeConfig(force);
   REGISTER_UNLOCK
}

