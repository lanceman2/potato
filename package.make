
# We are using quickbuild.make from
# https://raw.githubusercontent.com/lanceman2/quickbuild/$tag/quickbuild.make
# where $tag is master or a sha1 git hash we'll pin down if we need to.

# quickbuild.make reads this file for package specific configuration
# and config.make for build instance configuration.

VERSION = 0.0

# DEBUG is a global build option that affects
# how the library code is compiled and it's
# interfaces in the include files.
#
# Comment this out and set or not in config.make
DEBUG := yes # set here or not for now

IN_VARS := PO_DEBUG

ifdef DEBUG
PO_DEBUG := \#define PO_DEBUG
CPPFLAGS += -DPO_DEBUG
else
PO_DEBUG := //\#define PO_DEBUG is not defined in this build
endif


PREFIX ?= $(HOME)/installed/potato
