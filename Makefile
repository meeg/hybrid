# Variables
CC     := g++
DEF    :=
BIN    := $(PWD)/bin
OBJ    := $(PWD)/.obj
CFLAGS := -Wall `xml2-config --cflags` -I$(PWD)/generic -I$(PWD)/tracker -I$(PWD)/offline
LFLAGS := `xml2-config --libs` -lpthread -lrt

# Generic Sources
GEN_DIR := $(PWD)/generic
GEN_SRC := $(wildcard $(GEN_DIR)/*.cpp)
GEN_HDR := $(wildcard $(GEN_DIR)/*.h)
GEN_OBJ := $(patsubst $(GEN_DIR)/%.cpp,$(OBJ)/%.o,$(GEN_SRC))

# Tracker Sources
TRK_DIR := $(PWD)/tracker
TRK_SRC := $(wildcard $(TRK_DIR)/*.cpp)
TRK_HDR := $(wildcard $(TRK_DIR)/*.h)
TRK_OBJ := $(patsubst $(TRK_DIR)/%.cpp,$(OBJ)/%.o,$(TRK_SRC))

# Util Sources
UTL_DIR := $(PWD)/util
UTL_SRC := $(wildcard $(UTL_DIR)/*.cpp)
UTL_BIN := $(patsubst $(UTL_DIR)/%.cpp,$(BIN)/%,$(UTL_SRC))

# Default
all: dir $(GEN_OBJ) $(TRK_OBJ) $(UTL_BIN) gui ana online

# Object directory
dir:
	test -d $(OBJ) || mkdir $(OBJ)

# Clean
clean:
	rm -rf $(OBJ)
	rm -f $(BIN)/*
	cd root; make clean
	cd cntrlGui; make clean
	cd onlineGui; make clean

# Compile Common Sources
$(OBJ)/%.o: $(GEN_DIR)/%.cpp $(GEN_DIR)/%.h
	$(CC) -c $(CFLAGS) $(DEF) -o $@ $<

# Compile Tracker Sources
$(OBJ)/%.o: $(TRK_DIR)/%.cpp $(TRK_DIR)/%.h
	$(CC) -c $(CFLAGS) $(DEF) -o $@ $<

# Comile utilities
$(BIN)/%: $(UTL_DIR)/%.cpp $(GEN_OBJ) $(TRK_OBJ)
	$(CC) $(CFLAGS) $(LFLAGS) $(DEF) $(OBJ)/* -o $@ $<

# Compile gui
gui:
	cd cntrlGui; qmake
	cd cntrlGui; make

# Compile line display
online:
	cd onlineGui; qmake
	cd onlineGui; make

# Compile root
ana:
	cd root; make

# Generate documentation
docs: 
	cd ./doc; doxygen doxygen.cfg
	cd ./doc/latex; make
	cp ./doc/latex/refman.pdf ./doc/tracker_software.pdf 
