lib_LTLIBRARIES = lowGravityFlag.la

lowGravityFlag_la_SOURCES = lowGravityFlag.cpp
lowGravityFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
lowGravityFlag_la_LDFLAGS = -module -avoid-version -shared
lowGravityFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.lowGravityFlag.txt \
	lowGravityFlag.sln \
	lowGravityFlag.vcxproj	\
	lowGravityFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
