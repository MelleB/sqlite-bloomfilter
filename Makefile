
CC  	 = gcc
CXX      = g++
CXXFLAGS = -Wall -g
MODFLAGS = -shared -fPIC 
INCPATH  = -I./src/ -I./src/sqlite/
TARGET   = ./bin/

SOURCES  = src/murmur3/murmur3.c 
MODSRC 	 = src/sqlitebloom.c
TESTSRC  = src/murmur_check.c

all: module

module:
	$(CC) $(CXXFLAGS) $(MODFLAGS) $(INCPATH) -o libbloom.so $(SOURCES) $(MODSRC)

murmur_check: 
	$(CXX) $(CXXFLAGS) $(SOURCES) $(TESTSRC) -o murmur
