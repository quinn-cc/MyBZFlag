lib_LTLIBRARIES = ctfOverseer.la

ctfOverseer_la_SOURCES = ctfOverseer.cpp
ctfOverseer_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
ctfOverseer_la_LDFLAGS = -module -avoid-version -shared
ctfOverseer_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.ctfOverseer.txt \
	ctfOverseer.sln \
	ctfOverseer.vcxproj	\
	ctfOverseer.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
