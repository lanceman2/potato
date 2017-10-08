INSTALL_DIR = $(PREFIX)/include

INSTALLED := potato.h
BUILD := potato.h

L := ../lib/

LIB_INC_FILES :=\
 _potato.h\
 $(L)murmurHash.h\
 $(L)threadPool.h

IN_VARS := VERSION


# This target/rule is automatically in quickbuild.make
# _potato.h: _potato.h.in

# The comments in the header file got so large that we strip them out of
# the installed header file so it's not so large.
potato.h: $(LIB_INC_FILES) GNUmakefile
	echo -e "// This is a generated file\n" > $@
	for i in $(LIB_INC_FILES); do\
	    echo "////////// FILE: $$i //////////" >> $@ ;\
	    $(CXX) -fpreprocessed -dD -P -E $$i -o - >> $@ ||\
	    exit 1 ; done


include ../quickbuild.make
