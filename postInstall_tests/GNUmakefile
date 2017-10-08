
# To test linking with the potato library via the pkg-config
# system, try running something like:
#
#  PKG_CONFIG_PATH=${HOME}/installed/potato/lib/pkgconfig make
#

SHELL := /bin/bash

# Get potato PREFIX
include ../config.make

PKG := PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

IS_INSTALLED :=\
 $(strip\
 $(shell [ -f $(PREFIX)/lib/pkgconfig/potato.pc ]\
   && echo "Yes")\
 )

ifeq ($(IS_INSTALLED),)
  $(error "potato is not installed in: $(PREFIX)")
endif

BUILD_NO_INSTALL := poTest_murmurHash

poTest_murmurHash_SOURCES := poTest_murmurHash.c
poTest_murmurHash_LDFLAGS := $(shell $(PKG) pkg-config --libs potato)
poTest_murmurHash_CPPFLAGS := $(shell $(PKG) pkg-config --cflags potato)



include ./quickbuild.make
