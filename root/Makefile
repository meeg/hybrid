# Variables
CFLAGS  := -Wall `xml2-config --cflags` `root-config --cflags` -I$(PWD)/../tracker -I$(PWD)/../generic
LFLAGS  := `xml2-config --libs` `root-config --libs` -lMinuit
CC      := g++
BIN     := $(PWD)/../bin
OBJ     := $(PWD)/.obj

# Generic Sources
GEN_DIR := $(PWD)/../generic
GEN_SRC := $(GEN_DIR)/Data.cpp $(GEN_DIR)/DataRead.cpp
GEN_HDR := $(GEN_DIR)/Data.h   $(GEN_DIR)/DataRead.h
GEN_OBJ := $(patsubst $(GEN_DIR)/%.cpp,$(OBJ)/%.o,$(GEN_SRC))

# Tracker Sources
TRK_DIR := $(PWD)/../tracker
TRK_SRC := $(TRK_DIR)/TrackerEvent.cpp $(TRK_DIR)/TrackerSample.cpp
TRK_HDR := $(TRK_DIR)/TrackerEvent.h   $(TRK_DIR)/TrackerSample.h
TRK_OBJ := $(patsubst $(TRK_DIR)/%.cpp,$(OBJ)/%.o,$(TRK_SRC))

# Root Sources
ROOT_DIR := $(PWD)
ROOT_SRC := $(wildcard $(ROOT_DIR)/*.cpp)
ROOT_BIN := $(patsubst $(ROOT_DIR)/%.cpp,$(BIN)/%,$(ROOT_SRC))

# Default
all: dir $(GEN_OBJ) $(OFF_OBJ) $(TRK_OBJ) $(ROOT_BIN)

# Object directory
dir:
	test -d $(OBJ) || mkdir $(OBJ)

# Clean
clean:
	rm -rf $(OBJ)

# Compile Generic Sources
$(OBJ)/%.o: $(GEN_DIR)/%.cpp $(GEN_DIR)/%.h
	$(CC) -c $(CFLAGS) $(DEF) -o $@ $<

# Compile Tracker Sources
$(OBJ)/%.o: $(TRK_DIR)/%.cpp $(TRK_DIR)/%.h
	$(CC) -c $(CFLAGS) $(DEF) -o $@ $<

# Comile root
$(BIN)/%: $(ROOT_DIR)/%.cpp $(OFF_OBJ) $(GEN_OBJ) $(TRK_OBJ)
	$(CC) $(CFLAGS) $(LFLAGS) $(DEF) $(OBJ)/* -o $@ $<

