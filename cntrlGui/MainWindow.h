//-----------------------------------------------------------------------------
// File          : MainWindow.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General purpose
//-----------------------------------------------------------------------------
// Description :
// Top level control window
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#ifndef __CONTROL_GUI_H__
#define __CONTROL_GUI_H__

#include <QWidget>
#include <QDomDocument>
using namespace std;

class SystemWindow;
class CommandWindow;
class VariableWindow;
class ScriptWindow;

class MainWindow : public QWidget {
   
   Q_OBJECT

      void cmdResStructure (QDomNode node);

   public:

      // Window
      SystemWindow    *systemWindow;
      CommandWindow   *commandWindow;
      VariableWindow  *statusWindow;
      VariableWindow  *configWindow;
      ScriptWindow    *scriptWindow;

      // Creation Class
      MainWindow ( QWidget *parent = NULL );

      // Delete
      ~MainWindow ( );

   public slots:

      void xmlMessage (QDomNode node);
};

#endif
