# Project Name
TARGET ?= libflock-bluemchen-example

DEBUG = 1
OPT = -O0

# Sources
C_SOURCES = ../../../libflock/src/libflock.c
CPP_SOURCES = vendor/kxmx_bluemchen/src/kxmx_bluemchen.cpp src/${TARGET}.cpp

USE_FATFS = 1

# Library Locations
LIBDAISY_DIR = ../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
