//-----------------------------------------------------------------------------
// File          : VariableWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Command window in top GUI
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __VARIABLE_WINDOW_H__
#define __VARIABLE_WINDOW_H__

#include <QWidget>
#include <QDomDocument>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QTabWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
using namespace std;

class VariableHolder;

class VariableWindow : public QWidget {
   
   Q_OBJECT

      // Type
      QString type_;

      // Top level widget
      QTreeWidget     *tree_;
      QTreeWidgetItem *cfg_;

      // Config holder
      vector<VariableHolder *> varList_;

      // Read status button
      QPushButton *read_;
      QPushButton *write_;
      QPushButton *verify_;

      // Process response
      bool cmdResStructure (QDomNode node, VariableHolder *holder, QTreeWidgetItem *item);
      void cmdResConfig    (QDomNode node, QString id);

   public:

      // Creation Class
      VariableWindow ( QString type, QWidget *parent = NULL );

      // Delete
      ~VariableWindow ( );

   public slots:

      void xmlMessage (QDomNode node);
      void readPressed();
      void writePressed();
      void verifyPressed();
      void helpPressed (QString name, QString desc);

   signals:

      void sendCommand(QString cmd);
      void sendConfig(QString cmd);

};

#endif
