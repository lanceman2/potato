
# To test linking with the potato library via the pkg-config
# system, try running something like:
#
#  PKG_CONFIG_PATH=${HOME}/installed/potato/lib/pkgconfig make
#

poTest_murmurHash_SOURCES := poTest_murmurHash.c
poTest_murmurHash_LDFLAGS := $(shell pkg-config --libs potato)
poTest_murmurHash_CPPFLAGS := $(shell pkg-config --cflags potato)



include ./quickbuild.make
