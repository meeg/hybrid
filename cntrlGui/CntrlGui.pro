
TEMPLATE = app
FORMS    = 
HEADERS  = XmlClient.h SystemWindow.h MainWindow.h CommandHolder.h CommandWindow.h VariableHolder.h VariableWindow.h ScriptWindow.h ScriptButton.h
SOURCES  = XmlClient.cpp CntrlGui.cpp SystemWindow.cpp MainWindow.cpp CommandHolder.cpp CommandWindow.cpp  VariableHolder.cpp VariableWindow.cpp ScriptWindow.cpp ScriptButton.cpp
TARGET   = ../bin/cntrlGui
QT       += network xml script

