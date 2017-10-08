
# We are using quickbuild.make from
# https://raw.githubusercontent.com/lanceman2/quickbuild/$tag/quickbuild.make
# where $tag is master or a sha1 git hash we'll pin down if we need to.

# quickbuild.make reads this file for package specific configuration
# and config.make for build instance configuration.

VERSION := 0.0

#https://stackoverflow.com/questions/1251999/how-can-i-replace-a-newline-n-using-sed


# DEBUG is a global build option that affects
# how the library code is compiled and it's
# interfaces in the include files.
#
# Comment this out and set or not in config.make
DEBUG := yes # set here or not for now

IN_VARS := $(IN_VARS) PO_DEBUG




ifdef DEBUG
PO_DEBUG := \#define PO_DEBUG
# If PO_DEBUG is defined there will be spew levels otherwise there
# will not be spew levels.  The default spew level may be defined
# by defining SPEW_LEVEL_DEBUG SPEW_LEVEL_INFO SPEW_LEVEL_WARN
# or SPEW_LEVEL_ERROR
CPPFLAGS :=  -DPO_DEBUG -DSPEW_LEVEL_DEBUG $(CPPFLAGS) 
else
PO_DEBUG := //\#define PO_DEBUG is not defined in this build
endif



PREFIX ?= $(HOME)/installed/potato
