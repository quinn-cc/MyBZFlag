lib_LTLIBRARIES = highGravityFlag.la

highGravityFlag_la_SOURCES = highGravityFlag.cpp
highGravityFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
highGravityFlag_la_LDFLAGS = -module -avoid-version -shared
highGravityFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.highGravityFlag.txt \
	highGravityFlag.sln \
	highGravityFlag.vcxproj	\
	highGravityFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
