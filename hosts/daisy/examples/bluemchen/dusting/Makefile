# Project Name
TARGET ?= signaletic-nehcmeulb-dusting

DEBUG = 0
OPT = -O3

# Sources
C_SOURCES += ../../../../../libsignaletic/vendor/tlsf/tlsf.c ../../../../../libsignaletic/src/libsignaletic.c
C_INCLUDES += -I../../../../../libsignaletic/vendor/tlsf -I../../../../../libsignaletic/include

CPP_SOURCES = ../../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.cpp ../../../src/signaletic-daisy-host.cpp ../../../src/daisy-bluemchen-host.cpp src/${TARGET}.cpp

USE_FATFS = 0

# Library Locations
LIBDAISY_DIR = ../../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
