
libpotato.so_SOURCES := debug.c murmurHash.c
libpotato.so_CPPFLAGS := -DDEBUG

INSTALL_DIR = $(PREFIX)/lib 


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

test_debug_DEBUG_SOURCES := debug.c test_debug.c
test_debug_DEBUG_CPPFLAGS := -DDEBUG

test_debug_DEBUG_D_SOURCES := debug.c test_debug.c
test_debug_DEBUG_D_CPPFLAGS := -DDEBUG -DSPEW_LEVEL_DEBUG

test_poMurmurHash_SOURCES := murmurHash.c
test_poMurmurHash_CPPFLAGS := -DTEST

test_random_SOURCES := test_random.c murmurHash.c

test_randSequence_SOURCES :=\
 test_randSequence.c\
 randSequence.c\
 murmurHash.c\
 debug.c
test_randomSequence_CPPFLAGS := -DDEBUG

endif


include ../quickbuild.make
