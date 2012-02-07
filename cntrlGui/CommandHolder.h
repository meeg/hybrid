//-----------------------------------------------------------------------------
// File          : CommandHolder.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Command holder
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __COMMAND_HOLDER_H__
#define __COMMAND_HOLDER_H__
#include <QString>
#include <vector>
#include <QDomDocument>
#include <QObject>
#include <QToolButton>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
using namespace std;

class CommandHolder : public QObject {

   Q_OBJECT

      // Tracking values
      QString fullName_; 
      QString shortName_; 
      bool    hasArg_;
      QString desc_;
      bool    hidden_;

      QLineEdit   *arg_;
      QToolButton *btn_;
      QToolButton *hlp_;

   public:

      // Creation Class
      CommandHolder ( );
      CommandHolder ( CommandHolder * );

      // Delete
      ~CommandHolder ( );

      // Has arg
      bool hasArg();

      // Is hidden
      bool isHidden();

      // Add device level information
      QString addDevice ( QDomNode node );

      // Add command level information
      void addCommand ( QDomNode node );

      // Parse ID string
      void parseId ( QString id );

      // Get ID string
      QString getId ();

      // Get XML string
      QString getXml (QString arg);

      // Get short name
      QString shortName ();

      // Set enabled state
      void setEnabled(bool state);

      // Update tree widget item with fields
      void setupItem (QTreeWidgetItem *item);

   public slots:

      void pressed();
      void hPressed();

   signals:

      void commandPressed(QString xml);
      void helpPressed(QString name, QString desc);

};

#endif
