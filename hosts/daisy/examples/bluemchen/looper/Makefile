# Project Name
TARGET ?= signaletic-bluemchen-looper

DEBUG = 0
OPT = -O3

# Sources
C_SOURCES += ../../../../../libsignaletic/vendor/tlsf/tlsf.c ../../../../../libsignaletic/src/libsignaletic.c ../../../src/signaletic-host.c ./src/looper.c
C_INCLUDES += -I../../../../../libsignaletic/vendor/tlsf -I../../../../../libsignaletic/include

CPP_SOURCES = ../../../src/signaletic-daisy-host.cpp ../../../src/sig-daisy-seed.cpp src/${TARGET}.cpp
CPP_INCLUDES += -I../../src/include

USE_FATFS = 1

# Library Locations
LIBDAISY_DIR = ../../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
