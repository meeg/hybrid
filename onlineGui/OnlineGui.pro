
TEMPLATE = app
FORMS    = 
HEADERS  = ../generic/Data.h ../tracker/TrackerEvent.h ../tracker/TrackerSample.h UdpServer.h MainWindow.h TrackerDisplayData.h
SOURCES  = ../generic/Data.cpp ../tracker/TrackerEvent.cpp ../tracker/TrackerSample.cpp OnlineGui.cpp UdpServer.cpp MainWindow.cpp TrackerDisplayData.cpp
TARGET   = ../bin/onlineGui
QT       += network xml
INCLUDEPATH += ../generic/ ../tracker/ ${QWTDIR}/include
LIBS        += -L${QWTDIR}/lib -lqwt 

