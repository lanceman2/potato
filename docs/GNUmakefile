
INSTALL_DIR = $(PREFIX)/share/docs

INSTALLED := html

IN_VARS := VERSION

BUILD_NO_INSTALL := html/index.html

# TODO: check to build error
html/index.html:
	doxygen

CLEANFILES := doxygen_sqlite3.db
CLEANDIRS := html latex

include ../quickbuild.make

# We must add REPO_VERSION after including quickbuild.make so that
# top_srcdir is defined.  This is a little ugly, but oh well.
REPO_VERSION := $(shell if [ -f $(top_srcdir)/.git/config ] ; then\
 (echo "-" &&\
 git rev-parse --abbrev-ref HEAD &&\
 echo ":" &&\
 git rev-list --count HEAD) |\
 sed -e ':a;N;$$!ba;s/[\n| ]//g' ; fi)

# This is how you call the function, AddInReplace, with the argument,
# REPO_VERSION, in GNU make:
$(eval $(call AddInReplace,REPO_VERSION))
