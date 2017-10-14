
# We are using quickbuild.make from
# https://raw.githubusercontent.com/lanceman2/quickbuild/$tag/quickbuild.make
# where $tag is master or a sha1 git hash we'll pin down if we need to.

# quickbuild.make reads this file for package specific configuration
# and config.make for build instance configuration.

VERSION := 0.0

#https://stackoverflow.com/questions/1251999/how-can-i-replace-a-newline-n-using-sed


# DEBUG is a global build option that affects how the library code is
# compiled and but does not change it's interfaces in the include files.
#
# Comment this out and set or not in config.make

CPPFLAGS := -DDEBUG -DSPEW_LEVEL_NOTICE $(CPPFLAGS)

CFLAGS := $(CFLAGS) -Wall -Werror -g

# Default installation prefix
PREFIX ?= $(HOME)/installed/potato
