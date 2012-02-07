//-----------------------------------------------------------------------------
// File          : ScriptWindow.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Script window in top GUI
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <string>
#include <QDomDocument>
#include <QObject>
#include <QHeaderView>
#include <QMessageBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QTextStream>
#include <QTimer>
#include "ScriptWindow.h"
#include "CommandHolder.h"
#include "VariableHolder.h"
using namespace std;

// Constructor
ScriptWindow::ScriptWindow ( QWidget *parent ) : QWidget (parent) {
   runWait_    = 1;
   waitStop_   = false;
   runEnable_  = false;
   pendConfig_ = "";
   runState_   = "";

   QVBoxLayout *vbox = new QVBoxLayout;
   setLayout(vbox);

   // Script group box
   QGroupBox *scriptBox = new QGroupBox("Scripts");
   vbox->addWidget(scriptBox);

   scripts_ = new QVBoxLayout;
   scriptBox->setLayout(scripts_);
   scripts_->setAlignment(Qt::AlignTop);

   // Status
   status_ = new QLineEdit();
   status_->setReadOnly(true);
   status_->setText("Idle");
   stateMessage("");
   vbox->addWidget(status_);

   // Buttons
   QHBoxLayout *hbox = new QHBoxLayout;
   vbox->addLayout(hbox);

   searchScripts_ = new QPushButton("Refresh");
   hbox->addWidget(searchScripts_);
   connect(searchScripts_,SIGNAL(pressed()),this,SLOT(searchScripts()));

   stopScript_ = new QPushButton("Stop Script");
   hbox->addWidget(stopScript_);
   connect(stopScript_,SIGNAL(pressed()),this,SLOT(stopScript()));

   searchScripts();
}

// Delete
ScriptWindow::~ScriptWindow ( ) { 

}

void ScriptWindow::cmdResStatus(QDomNode node) {
   while (! node.isNull() ) {
      if ( node.isElement() ) {

         // Run state
         if ( node.nodeName() == "RunState" ) {

            // Run does not match configured run value
            // Start script if we are waiting for stop
            if ( runEnable_ && node.firstChild().nodeValue() != runState_ && waitStop_ ) runScript();

            // Run state matches configured state, set flag to wait for stop
            else if ( runEnable_ && node.firstChild().nodeValue() == runState_ ) waitStop_ = true;
         }
      }
      node = node.nextSibling();
   }
}

void ScriptWindow::xmlMessage (QDomNode node) {
   while ( ! node.isNull() ) {
      if ( node.nodeName() == "status" ) cmdResStatus(node.firstChild());
      node = node.nextSibling();
   }
}

void ScriptWindow::searchScripts() {
   ScriptButton *nxtScript;
   QLayoutItem  *child;

   while ( (child = scripts_->takeAt(0)) != 0 ) {
      delete(child);
   }

   QDir scriptsDir(".");
   QStringList fileNames = scriptsDir.entryList(QStringList("*.js"),QDir::Files);

   foreach (QString fileName, fileNames) {
      nxtScript = new ScriptButton(fileName);
      connect(nxtScript,SIGNAL(loadScript(QString)),this,SLOT(loadScript(QString)));
      scripts_->addWidget(nxtScript);
   }
}

void ScriptWindow::stopScript() {
   if ( runEnable_ ) {
      runEnable_ = false;
      runState_  = "Stopped";
      waitStop_  = false;
      sendCommand("<SetRunState>Stopped</SetRunState>");
      status_->setText("Idle");
      stateMessage("");
   }
}

void ScriptWindow::loadScript(QString scriptName) {
   QString msg;

   if ( runEnable_ ) return;
   pendConfig_ = "";

   QFile file(scriptName);
   if ( ! file.open(QIODevice::ReadOnly) ) 
      cout << "ScriptWindow::loadScript -> Error Opening Script File " << endl;
   else {
      runEnable_ = true;
      QTextStream in(&file);
      in.setCodec("UTF-8");
      QString script = in.readAll();
      file.close();

      runIter_ = 0;
      runScript_ = scriptName;
      msg = QString("Script ").append(runScript_).append(" running! Iter: ").append(QString().setNum(runIter_)); 
      status_->setText(msg);
      stateMessage(msg);

      qsCntl_ = interpreter_.newQObject(this);
      interpreter_.globalObject().setProperty("gui",qsCntl_);
      interpreter_.evaluate(script);

      if (interpreter_.hasUncaughtException()) {
         cout << "ScriptWindow::loadScript -> Uncaught exception at line "
              << interpreter_.uncaughtExceptionLineNumber() << ": "
              << qPrintable(interpreter_.uncaughtException().toString()) << endl;
         status_->setText("Idle");
         stateMessage("");
         runEnable_ = false;
      }
      runEnable_ = true;

      // Send any config
      if ( pendConfig_ != "" ) {
         sendConfig(pendConfig_);
         pendConfig_ = "";
      }

      runScript();
   }
}

void ScriptWindow::runScript() {
   QScriptValue ret;
   QString msg;

   if ( !runEnable_ ) {
      status_->setText("Idle");
      stateMessage("");
      return;
   }

   msg = QString("Script ").append(runScript_).append(" running! Iter: ").append(QString().setNum(runIter_)); 
   status_->setText(msg);
   stateMessage(msg);

   ret = qsCntl_.property("run").call(qsCntl_);
   runIter_++;

   // Send any config
   if ( pendConfig_ != "" ) {
      sendConfig(pendConfig_);
      pendConfig_ = "";
   }

   if ( ret.toString() != "" ) {
      runState_ = ret.toString();
      runEnable_ = true;
      QTimer::singleShot(runWait_,this,SLOT(runTimeout()));
   }
   else {
      runEnable_ = false;
      status_->setText("Idle");
      stateMessage("");
   }
   waitStop_ = false;
}

void ScriptWindow::runTimeout() {
   QString cmd;

   cmd = "<SetRunState>";
   cmd.append(runState_);
   cmd.append("</SetRunState>");
   if ( runEnable_ ) sendCommand(cmd);
}

void ScriptWindow::setValue ( QString name, QString value ) {
   QString        cmd;
   VariableHolder varHold;
 
   varHold.parseId(name);
   varHold.updateValue(value);
   cmd = varHold.getXml();

   pendConfig_.append(cmd);
}

void ScriptWindow::sendCommand ( QString command, QString arg ) {
   QString       cmd;
   CommandHolder cmdHold;
 
   cmdHold.parseId(command);
   cmd = cmdHold.getXml(arg);

   if ( pendConfig_ != "" ) {
      sendConfigCommand(pendConfig_,cmd);
      pendConfig_ = "";
   }
   else sendCommand(cmd);
}

void ScriptWindow::setDefaults () {
   QString cmd;

   cmd = "<SetDefaults/>";

   if ( pendConfig_ != "" ) {
      sendConfigCommand(pendConfig_,cmd);
      pendConfig_ = "";
   }
   else sendCommand(cmd);
}

void ScriptWindow::loadConfig ( QString file ) {
   QString cmd;

   cmd = "<ReadXmlFile>";
   cmd.append(file);
   cmd.append("</ReadXmlFile>");

   if ( pendConfig_ != "" ) {
      sendConfigCommand(pendConfig_,cmd);
      pendConfig_ = "";
   }
   else sendCommand(cmd);
}

void ScriptWindow::saveConfig ( QString file ) {
   QString cmd;
   cmd = "<WriteConfigXml>";
   cmd.append(file);
   cmd.append("</WriteConfigXml>");

   if ( pendConfig_ != "" ) {
      sendConfigCommand(pendConfig_,cmd);
      pendConfig_ = "";
   }
   else sendCommand(cmd);
}

void ScriptWindow::openFile ( QString file ) {
   QString cfg;

   cfg = pendConfig_;
   pendConfig_ = "";

   cfg.append("<DataFile>");
   cfg.append(file);
   cfg.append("</DataFile>");
   sendConfigCommand(cfg,"<OpenDataFile/>");
}

void ScriptWindow::closeFile () {
   QString cmd;
   cmd = "<CloseDataFile/>";

   if ( pendConfig_ != "" ) {
      sendConfigCommand(pendConfig_,cmd);
      pendConfig_ = "";
   }
   else sendCommand(cmd);
}

void ScriptWindow::setRunParameters ( QString rate, uint count ) {
   QString cfg;

   cfg = pendConfig_;
   pendConfig_ = "";

   cfg.append("<RunRate>");
   cfg.append(rate);
   cfg.append("</RunRate>");
   cfg.append("<RunCount>");
   cfg.append(QString().setNum(count));
   cfg.append("</RunCount>");

   sendConfig(cfg);
}

void ScriptWindow::setRunWait ( uint time ) {
   runWait_ = time;
   if ( runWait_ == 0 ) runWait_ = 1;
}

uint ScriptWindow::iter() {
   return(runIter_);
}

