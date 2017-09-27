
INSTALL_DIR = $(PREFIX)/share/docs

INSTALLED := html

BUILD := html/index.html

# TODO: check to build error
html/index.html:
	doxygen

CLEANFILES := doxygen_sqlite3.db
CLEANDIRS := html latex



include ../quickbuild.make
