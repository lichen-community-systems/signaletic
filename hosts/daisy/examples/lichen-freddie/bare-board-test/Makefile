# Project Name
TARGET ?= lichen-freddie-bare-board-test

DEBUG = 1
OPT = -O0

# Sources
C_SOURCES += ../../../../../libsignaletic/vendor/tlsf/tlsf.c ../../../../../libsignaletic/src/libsignaletic.c
C_INCLUDES += -I../../../../../libsignaletic/vendor/tlsf -I../../../../../libsignaletic/include

CPP_INCLUDES += -I../vendor/lib -I../../../vendor/lib/dev -I../../../include -I/include
CPP_SOURCES = src/${TARGET}.cpp ../../../src/sig-daisy-seed.cpp

USE_FATFS = 0

# Library Locations
LIBDAISY_DIR = ../../../vendor/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
