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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <qwt_plot.h>
#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include "MainWindow.h"
using namespace std;

class MyZoomer: public QwtPlotZoomer {
   public:
      MyZoomer( QwtPlotCanvas *canvas ): QwtPlotZoomer( canvas ) {
         setTrackerMode( AlwaysOn );
      }

      virtual QwtText trackerTextF( const QPointF &pos ) const {
         QColor bg( Qt::white );
         bg.setAlpha( 200 );

         QwtText text = QwtPlotZoomer::trackerTextF( pos );
         text.setBackgroundBrush( QBrush( bg ) );
         return text;
      }
};

class ColorMap: public QwtLinearColorMap {
   public:
      ColorMap(): QwtLinearColorMap( Qt::darkCyan, Qt::red ) {
         addColorStop( 0.1, Qt::cyan );
         addColorStop( 0.6, Qt::green );
         addColorStop( 0.95, Qt::yellow );
      }
};

// Constructor
MainWindow::MainWindow ( QWidget *parent ) : QWidget (parent) {
   this->setWindowTitle("Tracker Online");
   dCount_ = 0;
   fpga_   = 0;
   hybrid_ = 0;

   QVBoxLayout *top = new QVBoxLayout;
   this->setLayout(top);

   plot_ = new QwtPlot;
   top->addWidget(plot_);

   hist_ = new QwtPlotSpectrogram();
   hist_->setColorMap(new ColorMap());
   hist_->setData(&data_);
   hist_->attach(plot_);
	plot_->plotLayout()->setAlignCanvasToScales(true);	

   QList<double> contourLevels;
   for ( double level = 0.5; level < 10.0; level += 1.0 ) contourLevels += level;
   hist_->setContourLevels( contourLevels );

   QwtScaleWidget *rightAxis = plot_->axisWidget( QwtPlot::yRight  );
   QwtScaleWidget *leftAxis  = plot_->axisWidget( QwtPlot::yLeft   );
   QwtScaleWidget *botAxis   = plot_->axisWidget( QwtPlot::xBottom );
   rightAxis->setTitle( "Intensity" );
   rightAxis->setColorBarEnabled( true );
   plot_->enableAxis( QwtPlot::yRight );
   leftAxis->setTitle( "Channel (apv*128+chan)" );
   botAxis->setTitle( "ADC Value" );

   plot_->setAxisScale(QwtPlot::yLeft,0,(128*5-1));   
   plot_->setAxisScale(QwtPlot::xBottom,0,16383);
   plot_->updateAxes();

   // LeftButton for the zooming
   // MidButton for the panning
   // RightButton: zoom out by 1
   // Ctrl+RighButton: zoom out to full size

   QwtPlotZoomer* zoomer = new MyZoomer( plot_->canvas() );
   zoomer->setMousePattern( QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier );
   zoomer->setMousePattern( QwtEventPattern::MouseSelect3, Qt::RightButton );

   QwtPlotPanner *panner = new QwtPlotPanner( plot_->canvas() );
   panner->setAxisEnabled( QwtPlot::yRight, false );
   panner->setMouseButton( Qt::MidButton );

   // Avoid jumping when labels with more/less digits
   // appear/disappear when scrolling vertically

   //const QFontMetrics fm( axisWidget( QwtPlot::yLeft )->font() );
   //QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
   //sd->setMinimumExtent( fm.width( "100.00" ) );

   const QColor c( Qt::darkBlue );
   zoomer->setRubberBandPen( c );
   zoomer->setTrackerPen( c );

   top->addWidget(new QLabel("LeftButton = Zoom Select,    MidButton = Pan,    RightButton = Zoom Out,    Ctrl+RightButton = Zoom Full"));

   QHBoxLayout *hbox = new QHBoxLayout;
   top->addLayout(hbox);

   countBox_ = new QLineEdit("0 Events");
   countBox_->setReadOnly(true);
   hbox->addWidget(countBox_);

   hbox->addWidget(new QLabel("Fpga Select:"));
   fpgaSelect_ = new QSpinBox();
   fpgaSelect_->setMinimum(0);
   fpgaSelect_->setMaximum(7);
   connect(fpgaSelect_,SIGNAL(valueChanged(int)),this,SLOT(clearPressed())); 
   hbox->addWidget(fpgaSelect_);

   hbox->addWidget(new QLabel("Hybrid Select:"));
   hybridSelect_ = new QSpinBox();
   hybridSelect_->setMinimum(0);
   hybridSelect_->setMaximum(3);
   connect(hybridSelect_,SIGNAL(valueChanged(int)),this,SLOT(clearPressed())); 
   hbox->addWidget(hybridSelect_);

   autoEnable_ = new QCheckBox("Auto Z");
   autoEnable_->setChecked(true);
   connect(autoEnable_,SIGNAL(stateChanged(int)),this,SLOT(reDraw())); 
   hbox->addWidget(autoEnable_);
   
   hbox->addWidget(new QLabel("Z Max:"));
   maxCount_ = new QSpinBox();
   maxCount_->setMinimum(1);
   maxCount_->setMaximum(10000);
   connect(maxCount_,SIGNAL(valueChanged(int)),this,SLOT(reDraw())); 
   hbox->addWidget(maxCount_);

   QPushButton *btn = new QPushButton("Reset Plot");
   connect(btn,SIGNAL(pressed()),this,SLOT(clearPressed())); 
   hbox->addWidget(btn);

   connect(&timer_,SIGNAL(timeout()),this,SLOT(reDraw()));
   timer_.start(500);

   status_.clear();
   config_.clear();
}

// Delete
MainWindow::~MainWindow ( ) { 
   status_.clear();
   config_.clear();
}

void MainWindow::xmlLevel (QDomNode node, QString level, bool config) {
   QString      local;
   QString      index;
   QString      value;
   QString      temp;

   while ( ! node.isNull() ) {

      // Process element
      if ( node.isElement() ) {
         local = level;

         // Append node name to id
         if ( local != "" ) local.append(":");
         local.append(node.nodeName());

         // Node has index
         if ( node.hasAttributes() ) {
            index = node.attributes().namedItem("index").nodeValue();
            local.append("(");
            local.append(index);
            local.append(")");
         }

         // Process child
         xmlLevel(node.firstChild(),local,config);
      }

      // Process text
      else if ( node.isText() ) {
         local = level;
         value = node.nodeValue();
         temp = value;

         // Strip all spaces and newlines
         temp.remove(QChar(' '));
         temp.remove(QChar('\n'));

         // Resulting string is non-blank
         if ( temp != "" ) {

            // Config
            if ( config ) config_[local] = value;

            // Status
            else status_[local] = value;
         }
      }

      // Next node
      node = node.nextSibling();
   }
}

void MainWindow::xmlStatus (QDomNode node) {
   xmlLevel(node,"",false);
}

void MainWindow::xmlConfig (QDomNode node) {
   xmlLevel(node,"",true);
}

void MainWindow::rxData (uint size, uint *data) {
   TrackerSample *sample;
   uint          channel;
   uint          x;
   uint          y;
   uint          value;

   event_.copy(data,size);
   dCount_++;

   for (x=0; x < event_.count(); x++) {
      if ( fpga_ == event_.fpgaAddress() ) {

         // Get sample
         sample  = event_.sample(x);

         if ( hybrid_ == sample->hybrid() ) {
            channel = (sample->apv() * 128) + (127-sample->channel());

            // Get sub samples
            for (y=0; y < 6; y++) {
               value = sample->value(y);
               data_.fill(value,channel);
            }
         }
      }
   }
}

void MainWindow::clearPressed() {
   data_.init();
   dCount_ = 0;
   fpga_   = fpgaSelect_->value();
   hybrid_ = hybridSelect_->value();
   //plot_->setAxisScale(QwtPlot::yLeft,0,(128*5-1));   
   //plot_->setAxisScale(QwtPlot::xBottom,0,16383);
   //plot_->updateAxes();
   reDraw();
}

void MainWindow::reDraw() {
   QString count;

   if ( autoEnable_->isChecked() ) data_.setMax(0);
   else data_.setMax(maxCount_->value());

   QwtInterval zInterval = hist_->data()->interval( Qt::ZAxis );

   QwtScaleWidget *rightAxis = plot_->axisWidget( QwtPlot::yRight );
   rightAxis->setColorMap( zInterval, new ColorMap() );

   plot_->setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
   plot_->replot();

   count.setNum(dCount_);
   count.append(" Events");
   countBox_->setText(count);
}

