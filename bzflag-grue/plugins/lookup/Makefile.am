lib_LTLIBRARIES = lookup.la

lookup_la_SOURCES = lookup.cpp
lookup_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
lookup_la_LDFLAGS = -module -avoid-version -shared
lookup_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.lookup.txt \
	lookup.sln \
	lookup.vcxproj	\
	lookup.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
