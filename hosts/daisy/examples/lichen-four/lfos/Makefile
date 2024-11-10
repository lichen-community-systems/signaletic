# Project Name
TARGET ?= lichen-four-lfos

DEBUG = 0
OPT = -O3

# Sources
C_SOURCES += ../../../../../libsignaletic/vendor/tlsf/tlsf.c ../../../../../libsignaletic/src/libsignaletic.c ../../../src/signaletic-host.c
C_INCLUDES += -I../../../../../libsignaletic/vendor/tlsf -I../../../../../libsignaletic/include

CPP_INCLUDES += -I../vendor/lib -I../../../vendor/lib/dev -I../../../include
CPP_SOURCES = ../../../src/signaletic-daisy-host.cpp ../../../src/sig-daisy-patch-sm.cpp src/${TARGET}.cpp

USE_FATFS = 0

# Library Locations
LIBDAISY_DIR = ../../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
