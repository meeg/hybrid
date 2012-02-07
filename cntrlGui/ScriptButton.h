//-----------------------------------------------------------------------------
// File          : ScriptButton.h
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
#ifndef __SCRIPT_BUTTON_H__
#define __SCRIPT_BUTTON_H__

#include <QPushButton>
using namespace std;

class ScriptButton : public QPushButton {
   
   Q_OBJECT

      // Script name
      QString file_;

   public:

      // Creation Class
      ScriptButton ( QString name, QWidget *parent = NULL );

      // Delete
      ~ScriptButton ( );

   public slots:

      void scriptPressed();

   signals:

      void loadScript(QString file);

};

#endif
