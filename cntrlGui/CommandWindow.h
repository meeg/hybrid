//-----------------------------------------------------------------------------
// File          : CommandWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
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
#ifndef __COMMAND_WINDOW_H__
#define __COMMAND_WINDOW_H__

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
#include "CommandHolder.h"
using namespace std;

class CommandWindow : public QWidget {
   
   Q_OBJECT

      // Top level widget
      QTreeWidget     *tree_;
      QTreeWidgetItem *cfg_;

      // Command list
      vector<CommandHolder *> cmdList_;

      // Process response
      bool cmdResStructure (QDomNode node, CommandHolder *holder, QTreeWidgetItem *item);

   public:

      // Creation Class
      CommandWindow ( QWidget *parent = NULL );

      // Delete
      ~CommandWindow ( );

   public slots:

      void commandPressed (QString xml);

      void xmlMessage (QDomNode node);

      void helpPressed (QString name, QString desc);

   signals:

      void sendCommand(QString cmd);

};

#endif
