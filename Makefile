# 
# Makefile
#

ROOT := .
include $(ROOT)/Common.mk

all: tool

tool:
	$(MAKE) --dir=src

install: tool
	$(MKDIR) $(DESTDIR)/usr/include/scribble
	$(MKDIR) $(DESTDIR)/usr/bin
	$(INSTALL) $(INCLUDE_DIR)/scribble/*.h $(DESTDIR)/usr/include/scribble
	$(INSTALL) $(BIN_DIR)/* $(DESTDIR)/usr/bin

pkg-deb: all
	dpkg-buildpackage -us -uc -rfakeroot

include $(ROOT)/Rules.mk
