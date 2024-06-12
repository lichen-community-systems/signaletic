# Project Name
TARGET ?= signaletic-bluemchen-calibrator

DEBUG = 1
OPT = -O0

# Sources
C_SOURCES += ../../../../../libsignaletic/vendor/tlsf/tlsf.c ../../../../../libsignaletic/src/libsignaletic.c ../../../src/signaletic-host.c
C_INCLUDES += -I../../../../../libsignaletic/vendor/tlsf -I../../../../../libsignaletic/include

CPP_SOURCES = ../../../src/signaletic-daisy-host.cpp ../../../src/sig-daisy-seed.cpp src/${TARGET}.cpp

USE_FATFS = 0

# Library Locations
LIBDAISY_DIR = ../../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
