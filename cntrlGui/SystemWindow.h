//-----------------------------------------------------------------------------
// File          : SystemWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// System window in top GUI
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __SYSTEM_WINDOW_H__
#define __SYSTEM_WINDOW_H__

#include <QWidget>
#include <QDomDocument>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QObject>
#include <QTextEdit>
#include <QProgressBar>
using namespace std;

class CommandHolder;

class SystemWindow : public QWidget {
   
   Q_OBJECT

      // Window groups
      QGroupBox *counterBox();
      QGroupBox *configBox();
      QGroupBox *dataBox();
      QGroupBox *cmdBox();

      // Objects
      QLineEdit        *countDataRx_;
      QLineEdit        *countDataFile_;
      QLineEdit        *countUnexp_;
      QLineEdit        *countRegRx_;
      QLineEdit        *countTimeout_;
      QLineEdit        *countError_;
      QLineEdit        *countRun_;
      QPushButton      *setDefaults_;
      QPushButton      *configRead_;
      QPushButton      *configSave_;
      QPushButton      *refreshState_;
      QTextEdit        *stateLine_;
      QPushButton      *softReset_;
      QPushButton      *hardReset_;
      QLineEdit        *dataFile_;
      QPushButton      *dataBrowse_;
      QPushButton      *dataOpen_;
      QPushButton      *dataClose_;
      QComboBox        *runRate_;
      QComboBox        *runState_;
      QSpinBox         *runCount_;
      QProgressBar     *runProgress_;

      // Process response
      void cmdResStatus    (QDomNode node);
      void cmdResStructure (QDomNode node);
      void cmdResConfig    (QDomNode node);

      // Holders
      QString lastLoadSettings_;
      QString lastSaveSettings_;
      QString lastData_;

      // states
      QString stateMsg_;
      QString scriptMsg_;

      // Send command along with system window config
      void topConfigCommand(QString cmd);

   public:

      // Creation Class
      SystemWindow ( QWidget *parent = NULL );

      // Delete
      ~SystemWindow ( );

   public slots:

      void setDefaultsPressed();
      void configReadPressed();
      void configSavePressed();
      void refreshStatePressed();
      void browseDataPressed();
      void openDataPressed();
      void closeDataPressed();
      void resetCountPressed();
      void runStateActivated(const QString &);
      void hardResetPressed();
      void softResetPressed();

      void xmlMessage      (QDomNode node);
      void scriptMessage   (QString msg);
      void updateState     ();

   signals:

      void sendCommand(QString cmd);
      void sendConfigCommand(QString cfg, QString cmd);
      void stopScript();
};

#endif
