
libpotato.so_SOURCES := debug.c murmurHash.c threadPool.c

INSTALL_DIR = $(PREFIX)/lib 

CPPFLAGS := -DSPEW_LEVEL_DEBUG

# uncomment TESTS to build tests
# or
# comment out TESTS to not build tests
TESTS :=\
 test_debug\
 test_debug_DEBUG\
 test_debug_DEBUG_D\
 test_poMurmurHash

ifdef TESTS

BUILD_NO_INSTALL := $(TESTS)
test_debug_SOURCES := debug.c test_debug.c

test_debug_LEVEL_DEBUG_SOURCES := debug.c test_debug.c
test_debug_LEVEL_DEBUG_CPPFLAGS := -DSPEW_LEVEL_DEBUG

test_poMurmurHash_SOURCES := murmurHash.c
test_poMurmurHash_CPPFLAGS := -DTEST

test_time_SOURCES := test_time.c debug.c
test_time_CPPFLAGS := -DSPEW_LEVEL_DEBUG

test_random_SOURCES := test_random.c murmurHash.c

test_randSequence_SOURCES :=\
 test_randSequence.c\
 randSequence.c\
 murmurHash.c\
 debug.c

endif


include ../quickbuild.make
