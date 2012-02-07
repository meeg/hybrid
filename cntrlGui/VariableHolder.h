//-----------------------------------------------------------------------------
// File          : VariableHolder.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Variable holder
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __VARIABLE_HOLDER_H__
#define __VARIABLE_HOLDER_H__
#include <QString>
#include <vector>
#include <QDomDocument>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QToolButton>
using namespace std;

class VariableHolder : public QObject {

   Q_OBJECT

      // Tracking values
      QString         fullName_; 
      QString         shortName_; 
      QString         type_; 
      vector<QString> enums_;
      bool            compValid_;
      double          compA_;
      double          compB_;
      double          compC_;
      QString         compUnits_;
      uint            min_;
      uint            max_;
      QString         desc_;
      QString         value_;
      bool            hidden_;

   public:

      // Widgets
      QLineEdit   *input_;
      QSpinBox    *inBox_;
      QLineEdit   *output_;
      QComboBox   *select_;
      QToolButton *hlp_;

      // Creation Class
      VariableHolder ( );
      VariableHolder ( VariableHolder * );

      // Delete
      ~VariableHolder ( );

      // Get type match
      bool typeMatch(QString typeIn);

      // Add device level information
      QString addDevice ( QDomNode node );

      // Add command level information
      void addVariable ( QDomNode node );

      // Parse ID string
      void parseId ( QString id );

      // Get ID string
      QString getId ();

      // Get XML string
      QString getXml ();

      // Get enum list
      vector<QString> getEnums();

      // Get short name
      QString shortName ();

      // Get hidden status
      bool isHidden();

      // Update value
      void updateValue(QString value);

      // Set enabled state
      void setEnabled(bool state);

      // Update tree widget item with fields
      void setupItem (QTreeWidgetItem *item);

   public slots:

      // Compute output value
      void computeOutput();
      void hPressed();

   signals:

      void helpPressed(QString name, QString desc);

};

#endif
