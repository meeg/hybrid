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
#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMap>
#include <QDomDocument>
#include <QSpinBox>
#include <QTimer>
#include <QLineEdit>
#include <QCheckBox>
#include <TrackerEvent.h>
#include <TrackerDisplayData.h>
#include <qwt_plot_spectrogram.h>
using namespace std;

class MainWindow : public QWidget {
  
      QMap <QString, QString> config_;
      QMap <QString, QString> status_;
      TrackerEvent            event_;

      // Histogram
      QwtPlotSpectrogram *hist_;
      QwtPlot            *plot_;
      TrackerDisplayData data_;

      // Refresh timer
      QTimer timer_;

      uint fpga_;
      uint hybrid_;
      uint dCount_;

      QLineEdit *countBox_;
      QSpinBox  *fpgaSelect_;
      QSpinBox  *hybridSelect_;
      QCheckBox *autoEnable_;
      QSpinBox  *maxCount_;
 
   Q_OBJECT

      void xmlLevel (QDomNode node, QString level, bool config);

   public:

      // Window
      MainWindow ( QWidget *parent = NULL );

      // Delete
      ~MainWindow ( );

   public slots:

      void clearPressed();
      void reDraw();
      void xmlStatus (QDomNode node);
      void xmlConfig (QDomNode node);
      void rxData (uint size, uint *data);
};

#endif
