
INSTALL_DIR = $(PREFIX)/share/docs

INSTALLED := html

IN_VARS := VERSION

# We pick one of the generated files as the target
BUILD_NO_INSTALL := html/index.html

threadPool_drawing_sm.png: threadPool_drawing.png
	convert $< -transparent white -resize "50%" $@

threadPool_drawing.png: threadPool_drawing.odg
	libreoffice --invisible --convert-to png $<
	[ -f $@ ] || exit 1

html/index.html: threadPool_drawing_sm.png Doxyfile
	rm -rf html latex
	rm -f doxygen_sqlite3.db
	doxygen
	cp threadPool_drawing.png html/

CLEANFILES := doxygen_sqlite3.db threadPool_drawing.png threadPool_drawing_sm.png
CLEANDIRS := html latex

include ../quickbuild.make

html/index.html: $(wildcard $(top_srcdir)/lib/*.h)

# We must add REPO_VERSION after including quickbuild.make so that
# top_srcdir is defined.  This is a little ugly, but oh well.
#
# Maybe we could set this REPO_VERSION in a sottware build configure step.
REPO_VERSION := $(shell if [ -f $(top_srcdir)/.git/config ] ; then\
 (echo "-" &&\
 git rev-parse --abbrev-ref HEAD &&\
 echo ":" &&\
 git rev-list --count HEAD) |\
 sed -e ':a;N;$$!ba;s/[\n| ]//g' ; fi)

# This is how you call the function, AddInReplace, with the argument,
# REPO_VERSION, in GNU make.  AddInReplace is defined in quickbuild.
$(eval $(call AddInReplace,REPO_VERSION))
