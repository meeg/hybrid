//-----------------------------------------------------------------------------
// File          : CommandWindow.cpp
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
#include <QToolButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "CommandWindow.h"
using namespace std;

// Constructor
CommandWindow::CommandWindow ( QWidget *parent ) : QWidget (parent) {

   QVBoxLayout *vbox = new QVBoxLayout;
   setLayout(vbox);

   tree_ = new QTreeWidget();
   vbox->addWidget(tree_);

   tree_->setColumnCount(3);
   QStringList hdr;
   hdr << "Command" << "Arg" << "Execute" << "Info";
   tree_->setHeaderLabels(hdr);

   cfg_ = new QTreeWidgetItem(tree_);
   cfg_->setText(0,"Commands");
   tree_->addTopLevelItem(cfg_);
   cfg_->setExpanded(true);
}

// Delete
CommandWindow::~CommandWindow ( ) { 

}

bool CommandWindow::cmdResStructure (QDomNode node, CommandHolder *holder, QTreeWidgetItem *item ) {
   CommandHolder   *local = NULL;
   QString         temp;
   QTreeWidgetItem *next;
   bool            ret = false;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {

         // copy or create holder
         if ( holder == NULL ) local = new CommandHolder; 
         else local = new CommandHolder(holder);

         // Device found
         if ( node.nodeName() == "device" ) {

            // Add to local get name
            temp = local->addDevice(node.firstChild());

            // Create item
            next = new QTreeWidgetItem(item);
            next->setText(0,temp);
            item->addChild(next);
            item->setExpanded(true);

            // Process next level
            if ( !cmdResStructure(node.firstChild(),local,next) ) {
               item->removeChild(next);
               delete next;
            }
            else ret = true;
         }

         // Command found
         else if ( node.nodeName() == "command" ) {
            local->addCommand(node.firstChild());
            if ( ! local->isHidden() ) {

               QTreeWidgetItem *temp = new QTreeWidgetItem(item);
               local->setupItem(temp);
               item->addChild(temp);
               item->setExpanded(true);
               connect(local,SIGNAL(commandPressed(QString)),this,SLOT(commandPressed(QString)));
               connect(local,SIGNAL(helpPressed(QString,QString)),this,SLOT(helpPressed(QString,QString)));

               cmdList_.push_back(local); 
               if ( holder == NULL ) local = new CommandHolder; 
               else local = new CommandHolder(holder);
               ret = true;
            }
         }
         delete local;
      }
      node = node.nextSibling();
   }
   return(ret);
}

void CommandWindow::xmlMessage (QDomNode node) {
   while ( ! node.isNull() ) {
      if ( node.nodeName() == "structure" ) {
         cmdResStructure(node.firstChild(),NULL,cfg_);
         tree_->resizeColumnToContents(0);
      }
      node = node.nextSibling();
   }
}

void CommandWindow::commandPressed(QString xml) {
   sendCommand(xml);
}

void CommandWindow::helpPressed (QString name, QString desc) {
   QMessageBox::information(this,name,desc,QMessageBox::Ok);
}

