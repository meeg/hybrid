//-----------------------------------------------------------------------------
// File          : VariableWindow.cpp
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
#include "VariableWindow.h"
#include "VariableHolder.h"
using namespace std;

// Constructor
VariableWindow::VariableWindow ( QString type, QWidget *parent ) : QWidget (parent) {

   type_ = type;

   QVBoxLayout *vbox = new QVBoxLayout;
   setLayout(vbox);

   tree_ = new QTreeWidget();
   vbox->addWidget(tree_);

   tree_->setColumnCount(3);
   QStringList hdr;
   hdr << "Variable" << "Value" << "Decode" << "Info";
   tree_->setHeaderLabels(hdr);

   cfg_ = new QTreeWidgetItem(tree_);
   cfg_->setText(0,type_);
   tree_->addTopLevelItem(cfg_);
   cfg_->setExpanded(true);

   QHBoxLayout *hbox = new QHBoxLayout;
   vbox->addLayout(hbox);

   read_ = new QPushButton(QString("Read ").append(type_));
   hbox->addWidget(read_);
   connect(read_,SIGNAL(pressed()),this,SLOT(readPressed()));

   if ( type_ != "Status" ) {
      write_ = new QPushButton(QString("Write ").append(type_));
      hbox->addWidget(write_);
      connect(write_,SIGNAL(pressed()),this,SLOT(writePressed()));

      verify_ = new QPushButton(QString("Verify ").append(type_));
      hbox->addWidget(verify_);
      connect(verify_,SIGNAL(pressed()),this,SLOT(verifyPressed()));
   }
}

// Delete
VariableWindow::~VariableWindow ( ) { 

}

bool VariableWindow::cmdResStructure (QDomNode node, VariableHolder *holder, QTreeWidgetItem *item ) {
   VariableHolder   *local = NULL;
   QString         temp;
   QTreeWidgetItem *next;
   bool            ret = false;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {

         // copy or create holder
         if ( holder == NULL ) local = new VariableHolder; 
         else local = new VariableHolder(holder);

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
            if ( ! cmdResStructure(node.firstChild(),local,next) ) {
               item->removeChild(next);
               delete next;
            }
            else ret = true;
         }

         // Variable found
         else if ( node.nodeName() == "variable" ) {
            local->addVariable(node.firstChild());
            if ( (!local->isHidden()) && local->typeMatch(type_) ) {

               QTreeWidgetItem *temp = new QTreeWidgetItem(item);
               local->setupItem(temp);
               item->addChild(temp);
               item->setExpanded(true);
               connect(local,SIGNAL(helpPressed(QString,QString)),this,SLOT(helpPressed(QString,QString)));

               varList_.push_back(local); 
               if ( holder == NULL ) local = new VariableHolder; 
               else local = new VariableHolder(holder);
               ret = true;
            }
         }
         delete local;
      }
      node = node.nextSibling();
   }
   return(ret);
}

void VariableWindow::cmdResConfig (QDomNode node, QString id) {
   uint     x;
   QString  locId;
   QString  index;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {
         locId = id;

         // Append node name to id
         if ( locId != "" ) locId.append(":");
         locId.append(node.nodeName());

         // Node has index
         if ( node.hasAttributes() ) {
            index = node.attributes().namedItem("index").nodeValue();
            locId.append("(");
            locId.append(index);
            locId.append(")");
         }

         // Check for match
         for (x=0; x < varList_.size(); x++ ) {
            if ( varList_[x]->getId() == locId ) 
               varList_[x]->updateValue(node.firstChild().nodeValue());
         }

         // Process child
         cmdResConfig(node.firstChild(),locId);
      }
      node = node.nextSibling();
   }
}


void VariableWindow::xmlMessage (QDomNode node) {
   QString ret;

   while ( ! node.isNull() ) {
      if ( node.nodeName() == "structure" ) {
         cmdResStructure(node.firstChild(),NULL,cfg_);
         tree_->resizeColumnToContents(0);
      }
      else if ( node.nodeName() == "config" ) cmdResConfig(node.firstChild(),"");
      else if ( node.nodeName() == "status" ) cmdResConfig(node.firstChild(),"");

      node = node.nextSibling();
   }
}

void VariableWindow::readPressed() {
   QString cmd;
   if ( type_ == "Status" ) cmd = "<ReadStatus/>\n";
   else cmd = "<ReadConfig/>\n";
   sendCommand(cmd);
}

void VariableWindow::writePressed() {
   QString cmd;
   uint    x;

   for (x=0; x < varList_.size(); x++ ) cmd.append(varList_[x]->getXml());
   sendConfig(cmd);
}

void VariableWindow::verifyPressed() {
   QString cmd;
   cmd = "<VerifyConfig/>\n";
   sendCommand(cmd);
}

void VariableWindow::helpPressed (QString name, QString desc) {
   QMessageBox::information(this,name,desc,QMessageBox::Ok);
}

