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
#include "CommandHolder.h"
#include <iostream>
#include <QStringList>
using namespace std;

// Creation Class
CommandHolder::CommandHolder ( ) : QObject() {
   fullName_    = "";
   shortName_   = "";
   hasArg_      = false;
   arg_         = NULL;
   btn_         = NULL;
   desc_        = "";
   hidden_      = false;
}

// Creation Class
CommandHolder::CommandHolder ( CommandHolder *hld ) : QObject() {
   fullName_    = hld->fullName_;
   shortName_   = hld->shortName_;
   hasArg_      = hld->hasArg_;
   arg_         = NULL;
   btn_         = NULL;
   desc_        = hld->desc_;
   hidden_      = hld->hidden_;
}

// Delete
CommandHolder::~CommandHolder ( ) {

}

// Has arg
bool CommandHolder::hasArg() {
   return(hasArg_);
}

// Hidden
bool CommandHolder::isHidden() {
   return(hidden_);
}

// Add device level information
QString CommandHolder::addDevice ( QDomNode node ) {
   QString name;
   QString temp;
   QString index;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {
         if      ( node.nodeName() == "name"  ) name = node.firstChild().nodeValue();
         else if ( node.nodeName() == "index" ) index = node.firstChild().nodeValue();
      }
      node = node.nextSibling();
   }

   temp.append(name);
   if ( index != "" ) {
      temp.append("(");
      temp.append(index);
      temp.append(")");
   }
   fullName_.append(temp);
   fullName_.append(":");
   return(temp);
}

// Add command level information
void CommandHolder::addCommand ( QDomNode node ) {
   while ( ! node.isNull() ) {
      if ( node.isElement() ) {
         if      ( node.nodeName() == "name"        ) shortName_.append(node.firstChild().nodeValue());
         else if ( node.nodeName() == "hasArg"      ) hasArg_ = true;
         else if ( node.nodeName() == "hidden"      ) hidden_ = true;
         else if ( node.nodeName() == "description" ) desc_ = node.firstChild().nodeValue();
      }
      node = node.nextSibling();
   }
   fullName_.append(shortName_);
}

// Parse ID string
void CommandHolder::parseId ( QString id ) {
   QStringList list;

   fullName_ = id;

   list = fullName_.split(":");

   // Process each list item
   shortName_ = list.at(list.size()-1);
}

// Get ID string
QString CommandHolder::getId () {
   return(fullName_);
}

// Get XML string
QString CommandHolder::getXml (QString arg) {
   QString     ret;
   int         x;
   QStringList tags;
   QStringList list;
   QStringList fields;

   list = fullName_.split(":");

   // Process each list item
   for (x=0; x < (list.size()-1); x++) {

      // Break into fields
      fields = list.at(x).split(QRegExp("[()]"));

      // XML tag start
      tags.push_back(fields.at(0));
      ret.append("<");
      ret.append(fields.at(0));

      // index if present
      if ( fields.size() > 1 ) {
         ret.append(" index=\"");
         ret.append(fields.at(1));
         ret.append("\"");
      }

      // XML tag end
      ret.append(">\n");
   }

   // Last one is the command
   ret.append("<");
   ret.append(list.at(list.size()-1));

   if ( arg == "" ) ret.append("/>\n");
   else {
      ret.append(">");
      ret.append(arg);
      ret.append("</");
      ret.append(list.at(list.size()-1));
      ret.append(">\n");
   }

   // Append end xml tags
   for (x=(tags.size()-1); x >= 0; x--) {
      ret.append("</");
      ret.append(tags.at(x));
      ret.append(">\n");
   }
   return(ret);
}

QString CommandHolder::shortName() { 
   return(shortName_);
}

void CommandHolder::setEnabled(bool state) { 
   if ( btn_ != NULL ) btn_->setEnabled(state);
}

// Setup tree widget item
void CommandHolder::setupItem (QTreeWidgetItem *item) {
   item->setText(0,shortName_);

   btn_ = new QToolButton;
   btn_->setText("Exec");
   connect(btn_,SIGNAL(pressed()),this,SLOT(pressed()));
   item->treeWidget()->setItemWidget(item,2,btn_); 

   if ( hasArg_ ) {
      arg_ = new QLineEdit;
      item->treeWidget()->setItemWidget(item,1,arg_); 
   }
 
   hlp_ = new QToolButton;
   hlp_->setText("Info");
   connect(hlp_,SIGNAL(pressed()),this,SLOT(hPressed()));
   item->treeWidget()->setItemWidget(item,3,hlp_); 
}

void CommandHolder::pressed() {
   QString tmp;

   if ( arg_ == NULL ) tmp = "";
   else tmp = arg_->text();
   commandPressed(getXml(tmp));
}

void CommandHolder::hPressed() {
   helpPressed(shortName_,desc_);
}

