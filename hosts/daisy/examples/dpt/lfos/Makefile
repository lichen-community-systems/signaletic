# Project Name
TARGET ?= signaletic-dpt-lfos

DEBUG = 1
OPT = -O0

# Sources
C_SOURCES += ../../../../../libsignaletic/vendor/tlsf/tlsf.c ../../../../../libsignaletic/src/libsignaletic.c
C_INCLUDES += -I../../../../../libsignaletic/vendor/tlsf -I../../../../../libsignaletic/include

CPP_INCLUDES += -I../vendor/lib -I../../../vendor/lib/dev -I../../include
CPP_SOURCES = src/${TARGET}.cpp ../../../vendor/dpt/lib/daisy_dpt.cpp ../../../vendor/dpt/lib/dev/DAC7554.cpp ../../../src/signaletic-daisy-host.cpp ../../../src/daisy-dpt-host.cpp

USE_FATFS = 1

# Library Locations
LIBDAISY_DIR = ../../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
