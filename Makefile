
CC  	 = gcc
CXX      = g++
CXXFLAGS = -Wall -g
MODFLAGS = -shared -fPIC 
INCPATH  = -I./src/ -I./src/sqlite/
TARGET   = ./bin/
CHK_DIR  = test -d
MKDIR    = mkdir -p

SOURCES  = src/murmur3/murmur3.c \
		   src/murmur3str.c 
MODSRC 	 = src/test2.c
TESTSRC  = src/murmur_check.c

all: module

module:
	$(CC) $(CXXFLAGS) $(MODFLAGS) $(INCPATH) -o libmurmur.so $(SOURCES) $(MODSRC)

murmur_check: 
	$(CXX) $(CXXFLAGS) $(SOURCES) $(TESTSRC) -o murmur
