#/******************************************************************************
# * easyvdr - A plugin for the Video Disk Recorder
# *
# * See the README file for copyright information and how to reach the author.
# *****************************************************************************/

PLUGIN = easyvdr

OBJS = easyvdr.o
OBJS+= EasyPluginManager.o
OBJS+= IniFile.o
OBJS+= MainMenu.o
OBJS+= DeviceManager.o


CMDOBJS = EasyControl.o
CMDOBJS+= IniFile.o


### The version number of this plugin (taken from the main source file):
VERSION = $(shell grep 'static constexpr const char\* s_version *= ' easyvdr.cpp | awk '{ print $$7 }' | sed -e 's/[";]//g')


### The directory environment:

# Use package data if installed...otherwise assume we're under the VDR source directory:
PKGCFG = $(if $(VDRDIR),$(shell pkg-config --variable=$(1) $(VDRDIR)/vdr.pc),$(shell PKG_CONFIG_PATH="$$PKG_CONFIG_PATH:../../.." pkg-config --variable=$(1) vdr))
LIBDIR = $(call PKGCFG,libdir)
LOCDIR = $(call PKGCFG,locdir)
PLGCFG = $(call PKGCFG,plgcfg)
#
TMPDIR ?= /tmp

### The compiler options:

export CFLAGS   = $(call PKGCFG,cflags)
export CXXFLAGS = $(call PKGCFG,cxxflags)

### The version number of VDR's plugin API:

APIVERSION = $(call PKGCFG,apiversion)

### Allow user defined options to overwrite defaults:

-include $(PLGCFG)

### The name of the distribution archive:

ARCHIVE = $(PLUGIN)-$(VERSION)
PACKAGE = vdr-$(ARCHIVE)

### The name of the shared object file:

SOFILE = libvdr-$(PLUGIN).so
CMDFILE = $(PLUGIN)ctl-$(APIVERSION)


### Includes and Defines (add further entries here):

INCLUDES += -I.

DEFINES += -DPLUGIN_NAME_I18N='"$(PLUGIN)"' -DAPIVERSION='"$(APIVERSION)"'
#DEFINES += -DDEBUG

#/******************************************************************************
# * dependencies, add variables here, and checks in target check_dependencies
# *****************************************************************************/
LIBREPFUNC=librepfunc
LIBREPFUNC_MINVERSION=1.1.0

# /* require either PKG_CONFIG_PATH to be set, or, a working pkg-config */
HAVE_LIBREPFUNC           =$(shell if pkg-config --exists                                   $(LIBREPFUNC); then echo "1"; else echo "0"; fi )
HAVE_LIBREPFUNC_MINVERSION=$(shell if pkg-config --atleast-version=$(LIBREPFUNC_MINVERSION) $(LIBREPFUNC); then echo "1"; else echo "0"; fi )
INCLUDES += $(shell pkg-config --cflags-only-I $(LIBREPFUNC))
LIBS     += $(shell pkg-config --libs-only-l $(LIBREPFUNC))
LDFLAGS  += $(shell pkg-config --libs-only-L $(LIBREPFUNC))



### The main target:

all: check_dependencies $(SOFILE) i18n cmd

### Implicit rules:

%.o: %.cpp
	@echo CC $@
	$(Q)$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) -o $@ $<

### Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(CXXFLAGS) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.cpp) > $@

-include $(DEPFILE)

### Internationalization (I18N):

PODIR     = po
I18Npo    = $(wildcard $(PODIR)/*.po)
I18Nmo    = $(addsuffix .mo, $(foreach file, $(I18Npo), $(basename $(file))))
I18Nmsgs  = $(addprefix $(DESTDIR)$(LOCDIR)/, $(addsuffix /LC_MESSAGES/vdr-$(PLUGIN).mo, $(notdir $(foreach file, $(I18Npo), $(basename $(file))))))
I18Npot   = $(PODIR)/$(PLUGIN).pot

%.mo: %.po
	@echo MO $@
	$(Q)msgfmt -c -o $@ $<

$(I18Npot): $(wildcard *.cpp)
	@echo GT $@
	$(Q)xgettext -C -cTRANSLATORS --no-wrap --no-location -k -ktr -ktrNOOP --package-name=vdr-$(PLUGIN) --package-version=$(VERSION) --msgid-bugs-address='<see README>' -o $@ `ls $^`

%.po: $(I18Npot)
	@echo PO $@
	$(Q)msgmerge -U --no-wrap --no-location --backup=none -q -N $@ $<
	@touch $@

$(I18Nmsgs): $(DESTDIR)$(LOCDIR)/%/LC_MESSAGES/vdr-$(PLUGIN).mo: $(PODIR)/%.mo
	install -D -m644 $< $@

.PHONY: i18n check_dependencies
i18n: $(I18Nmo) $(I18Npot)

install-i18n: $(I18Nmsgs)

### Targets:

cmd: $(CMDOBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CMDOBJS) $(LIBS) -o $(CMDFILE)

$(SOFILE): $(OBJS)
	@echo LD $@
	$(Q)$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@

install-lib: $(SOFILE)
	install -D $^ $(DESTDIR)$(LIBDIR)/$^.$(APIVERSION)

install-cmd: cmd
	cp $(CMDFILE) /usr/local/bin

install: install-lib install-i18n install-cmd

test:
	@echo \"$(VERSION)\"

dist: $(I18Npo) clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

clean:
	@-rm -f $(PODIR)/*.mo $(PODIR)/*.pot
	@-rm -f $(OBJS) $(CMDOBJS) $(DEPFILE) *.so *.tgz core* *~ $(CMDFILE)

#/******************************************************************************
# * dependencies, check them here and provide message to user.
# *****************************************************************************/
check_dependencies:
ifeq ($(HAVE_LIBREPFUNC),0)
	@echo "ERROR: dependency not found: $(LIBREPFUNC) >= $(LIBREPFUNC_MINVERSION)"
	exit 1
endif
ifeq ($(HAVE_LIBREPFUNC_MINVERSION),0)
	@echo "ERROR: dependency $(LIBREPFUNC) older than $(LIBREPFUNC_MINVERSION)"
	exit 1
endif
