//-----------------------------------------------------------------------------
// File          : ScriptWindow.h
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
#ifndef __SCRIPT_WINDOW_H__
#define __SCRIPT_WINDOW_H__

#include <QWidget>
#include <QDomDocument>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QObject>
#include <QScriptEngine>
#include <QScriptValue>
#include "ScriptButton.h"
using namespace std;

class ScriptWindow : public QWidget {
   
   Q_OBJECT

      // Buttons
      QPushButton *searchScripts_;
      QPushButton *stopScript_;
      QVBoxLayout *scripts_;
      QLineEdit   *status_;

      // Process response
      void cmdResStatus    (QDomNode node);

      QString       runScript_;
      uint          runIter_;
      uint          runWait_;
      bool          waitStop_;
      bool          runEnable_;
      QScriptEngine interpreter_;
      QScriptValue  qsCntl_;
      QString       pendConfig_;
      QString       runState_;

   public:

      // Creation Class
      ScriptWindow ( QWidget *parent = NULL );

      // Delete
      ~ScriptWindow ( );

   public slots:

      // Script methods
      void setValue         ( QString name,    QString value );
      void sendCommand      ( QString command, QString arg   );
      void setDefaults      ();
      void loadConfig       ( QString file );
      void saveConfig       ( QString file );
      void openFile         ( QString file );
      void closeFile        ();
      void setRunParameters ( QString rate, uint count );
      void setRunWait       ( uint time );
      uint iter             ();

      void xmlMessage      (QDomNode node);
      void loadScript(QString scriptName);
      void runScript();
      void searchScripts();
      void stopScript();
      void runTimeout();

   signals:

      void sendCommand(QString cmd);
      void sendConfigCommand(QString cfg, QString cmd);
      void sendConfig(QString cmd);
      void stateMessage(QString msg);

};

#endif
