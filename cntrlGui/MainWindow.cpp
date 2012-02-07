//-----------------------------------------------------------------------------
// File          : MainWindow.cpp
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
#include <iostream>
#include <sstream>
#include <string>
#include <QObject>
#include <QTabWidget>
#include <QVBoxLayout>
#include "MainWindow.h"
#include "SystemWindow.h"
#include "CommandWindow.h"
#include "VariableWindow.h"
#include "ScriptWindow.h"
using namespace std;

// Constructor
MainWindow::MainWindow ( QWidget *parent ) : QWidget (parent) {

   // Top level
   QVBoxLayout *base = new QVBoxLayout;
   this->setLayout(base);

   QTabWidget *tab = new QTabWidget;
   base->addWidget(tab);

   // Setup tabs
   systemWindow = new SystemWindow;
   tab->addTab(systemWindow,"System");

   commandWindow = new CommandWindow;
   tab->addTab(commandWindow,"Commands");

   statusWindow = new VariableWindow("Status");
   tab->addTab(statusWindow,"Status");

   configWindow = new VariableWindow("Configuration");
   tab->addTab(configWindow,"Configuration");

   scriptWindow = new ScriptWindow;
   tab->addTab(scriptWindow,"Scripts");

}

// Delete
MainWindow::~MainWindow ( ) { 

}

void MainWindow::xmlMessage (QDomNode node) {
   while ( ! node.isNull() ) {
      if ( node.nodeName() == "structure" ) cmdResStructure(node.firstChild());
      node = node.nextSibling();
   }
}

void MainWindow::cmdResStructure (QDomNode node) {
   while ( ! node.isNull() ) {
      if ( node.isElement() ) {
         if ( node.nodeName() == "description" ) {
            setWindowTitle(node.firstChild().nodeValue());
         }
      }
      node = node.nextSibling();
   }
   update();
}


