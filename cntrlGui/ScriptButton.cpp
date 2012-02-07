//-----------------------------------------------------------------------------
// File          : ScriptButton.cpp
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 10/04/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Script button
//-----------------------------------------------------------------------------
// Copyright (c) 2011 by SLAC. All rights reserved.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 10/04/2011: created
//-----------------------------------------------------------------------------
#include <QPushButton>
#include "ScriptButton.h"
using namespace std;

// Creation Class
ScriptButton::ScriptButton ( QString name, QWidget *parent ) : QPushButton(name,parent) {
   file_ = name; 
   connect(this,SIGNAL(pressed()),this,SLOT(scriptPressed()));
}

// Delete
ScriptButton::~ScriptButton ( ) { }

void ScriptButton::scriptPressed() {
   loadScript(file_);
}

