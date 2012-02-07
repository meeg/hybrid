//-----------------------------------------------------------------------------
// File          : VariableHolder.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
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
#include "VariableHolder.h"
#include <iostream>
#include <QStringList>
using namespace std;

// Creation Class
VariableHolder::VariableHolder ( ) : QObject() {
   fullName_    = "";
   shortName_   = "";
   type_        = "";
   input_       = NULL;
   inBox_       = NULL;
   output_      = NULL;
   select_      = NULL;
   compValid_   = false;
   compA_       = 0;
   compB_       = 0;
   compC_       = 0;
   compUnits_   = "";
   min_         = 0;
   max_         = 0;
   desc_        = "";
   value_       = "";
   hidden_      = false;
}

// Creation Class
VariableHolder::VariableHolder ( VariableHolder *in ) : QObject() {
   fullName_    = in->fullName_;
   shortName_   = in->shortName_;
   type_        = in->type_;
   enums_       = in->enums_;
   compValid_   = in->compValid_;
   compA_       = in->compA_;
   compB_       = in->compB_;
   compC_       = in->compC_;
   compUnits_   = in->compUnits_;
   min_         = in->min_;
   max_         = in->max_;
   input_       = NULL;
   inBox_       = NULL;
   output_      = NULL;
   select_      = NULL;
   desc_        = in->desc_;
   value_       = in->value_;
   hidden_      = in->hidden_;
}

// Delete
VariableHolder::~VariableHolder ( ) {

}

// Get type match
bool VariableHolder::typeMatch(QString typeIn) {
   if ( typeIn == "Configuration" && ( type_ == "Configuration" || type_ == "Feedback" )) return(true);
   if ( typeIn == "Status" && type_ == "Status" ) return(true);
   return(false);
}

// Add device level information
QString VariableHolder::addDevice ( QDomNode node ) {
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
void VariableHolder::addVariable ( QDomNode node ) {
   bool ok;

   while ( ! node.isNull() ) {
      if ( node.isElement() ) {
         if      ( node.nodeName() == "name"        ) shortName_.append(node.firstChild().nodeValue());
         else if ( node.nodeName() == "type"        ) type_ = node.firstChild().nodeValue();
         else if ( node.nodeName() == "enum"        ) enums_.push_back(node.firstChild().nodeValue());
         else if ( node.nodeName() == "min"         ) min_ = node.firstChild().nodeValue().toUInt(&ok,0);
         else if ( node.nodeName() == "max"         ) max_ = node.firstChild().nodeValue().toUInt(&ok,0);
         else if ( node.nodeName() == "compA"       ) compA_ = node.firstChild().nodeValue().toDouble();
         else if ( node.nodeName() == "compB"       ) compB_ = node.firstChild().nodeValue().toDouble();
         else if ( node.nodeName() == "compC"       ) compC_ = node.firstChild().nodeValue().toDouble();
         else if ( node.nodeName() == "description" ) desc_ = node.firstChild().nodeValue();
         else if ( node.nodeName() == "hidden"      ) hidden_ = true;
         else if ( node.nodeName() == "compUnits" ) {
            compUnits_ = node.firstChild().nodeValue();
            compValid_ = true;
         }
      }
      node = node.nextSibling();
   }
   fullName_.append(shortName_);
}

// Parse ID string
void VariableHolder::parseId ( QString id ) {
   QStringList list;

   fullName_ = id;

   list = fullName_.split(":");

   // Process each list item
   shortName_ = list.at(list.size()-1);
}

// Get ID string
QString VariableHolder::getId () {
   return(fullName_);
}

// Get ID string
bool VariableHolder::isHidden () {
   return(hidden_);
}

// Get XML string
QString VariableHolder::getXml () {
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

   // Start variable name
   ret.append("<");
   ret.append(list.at(list.size()-1));
   ret.append(">");

   // Variable value
   if ( input_  != NULL ) ret.append(input_->text());
   else if ( inBox_  != NULL ) ret.append(QString().setNum(inBox_->value()));
   else if ( select_ != NULL ) ret.append(select_->currentText());
   else ret.append(value_);

   // End variable name
   ret.append("</");
   ret.append(list.at(list.size()-1));
   ret.append(">\n");

   // Append end xml tags
   for (x=(tags.size()-1); x >= 0; x--) {
      ret.append("</");
      ret.append(tags.at(x));
      ret.append(">\n");
   }
   return(ret);
}

QString VariableHolder::shortName() { 
   return(shortName_);
}

// Update value
void VariableHolder::updateValue(QString value) {
   uint    idx;
   QString ret;
   bool    ok;

   value_ = value;

   // Update select
   if ( select_ != NULL ) {
      idx = select_->findText(value);
      select_->setCurrentIndex(idx);
   }

   // Update input
   if ( input_ != NULL ) input_->setText(value);
   if ( inBox_ != NULL ) inBox_->setValue(value.toUInt(&ok,0));
}

void VariableHolder::setEnabled(bool state) { 
   if ( select_ != NULL ) select_->setEnabled(state);
   if ( input_  != NULL ) input_->setEnabled(state);
   if ( inBox_  != NULL ) inBox_->setEnabled(state);
}

// Setup tree widget item
void VariableHolder::setupItem (QTreeWidgetItem *item) {
   uint x;

   item->setText(0,shortName_);

   // Choose between input box, spin box and Select box
   if ( enums_.size() > 0 && type_ == "Configuration" ) {
      select_ = new QComboBox;
      for (x = 0; x < enums_.size(); x++) select_->addItem(enums_[x]);
      item->treeWidget()->setItemWidget(item,1,select_);
   }
   else if ( min_ != max_ && type_ == "Configuration" ) {
      inBox_ = new QSpinBox;
      inBox_->setMinimum(min_);
      inBox_->setMaximum(max_);
      item->treeWidget()->setItemWidget(item,1,inBox_);
      connect(inBox_,SIGNAL(valueChanged(int)),this,SLOT(computeOutput()));

      if ( compValid_ ) {
         output_ = new QLineEdit;
         output_->setReadOnly(true);
         item->treeWidget()->setItemWidget(item,2,output_);
      }
   }
   else {
      input_ = new QLineEdit;
      if ( type_ == "Status" || type_ == "Feedback" ) input_->setReadOnly(true);
      item->treeWidget()->setItemWidget(item,1,input_);
      connect(input_,SIGNAL(textChanged(QString)),this,SLOT(computeOutput()));

      if ( compValid_ ) {
         output_ = new QLineEdit;
         output_->setReadOnly(true);
         item->treeWidget()->setItemWidget(item,2,output_);
      }
   }

   hlp_ = new QToolButton;
   hlp_->setText("Info");
   connect(hlp_,SIGNAL(pressed()),this,SLOT(hPressed()));
   item->treeWidget()->setItemWidget(item,3,hlp_); 

}

// Compute output value
void VariableHolder::computeOutput() {
   uint    value = 0;
   QString ret;
   double  val;
   bool    ok;

   if ( input_  != NULL ) value = input_->text().toInt(&ok,0);
   if ( inBox_  != NULL ) value = inBox_->value();

   if ( output_ != NULL ) {
      val = ((double(value) + compA_) * compB_) + compC_;
      ret.setNum(val);
      ret.append(" ");
      ret.append(compUnits_);
      output_->setText(ret);
   }
}

void VariableHolder::hPressed() {
   helpPressed(shortName_,desc_);
}

// Get enum list
vector<QString> VariableHolder::getEnums() {
   return(enums_);
}

