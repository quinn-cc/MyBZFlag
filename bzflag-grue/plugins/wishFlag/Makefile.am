lib_LTLIBRARIES = wishFlag.la

wishFlag_la_SOURCES = wishFlag.cpp
wishFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
wishFlag_la_LDFLAGS = -module -avoid-version -shared
wishFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.wishFlag.txt \
	wishFlag.sln \
	wishFlag.vcxproj	\
	wishFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
