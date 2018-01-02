IN_VARS := VERSION

libpotato.so_SOURCES := debug.c time.c murmurHash.c threadPool.c

# Reference:
# https://www.gnu.org/software/gnulib/manual/html_node/LD-Version-Scripts.html
#
# Run to see defined symbols:
#    nm -D libpotato.so | grep ' T '
#
libpotato.so_LDFLAGS := -Wl,--version-script=libpotato.export


INSTALL_DIR = $(PREFIX)/lib 



TESTS := yes

ifdef TESTS

BUILD_NO_INSTALL :=\
 test_debug\
 test_debug_DEBUG\
 test_debug_DEBUG_D\
 test_poMurmurHash

test_debug_SOURCES := debug.c test_debug.c

test_debug_LEVEL_DEBUG_SOURCES := debug.c test_debug.c

test_poMurmurHash_SOURCES := murmurHash.c
test_poMurmurHash_CPPFLAGS := -DTEST

test_time_SOURCES := test_time.c time.c debug.c
test_time_CPPFLAGS := -DSPEW_LEVEL_DEBUG

test_random_SOURCES := test_random.c murmurHash.c

test_randSequence_SOURCES :=\
 randSequence.c\
 test_randSequence.c\
 murmurHash.c\
 debug.c

endif


include ../quickbuild.make
