lib_LTLIBRARIES = ScoreRestorer.la

ScoreRestorer_la_SOURCES = ScoreRestorer.cpp
ScoreRestorer_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
ScoreRestorer_la_LDFLAGS = -module -avoid-version -shared
ScoreRestorer_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	LICENSE.md \
	README.md \
	ScoreRestorer.def \
	ScoreRestorer.sln \
	ScoreRestorer.vcxproj \
	ScoreRestorer.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
