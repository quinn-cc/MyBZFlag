lib_LTLIBRARIES = ricoscoreFlag.la

ricoscoreFlag_la_SOURCES = ricoscoreFlag.cpp
ricoscoreFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
ricoscoreFlag_la_LDFLAGS = -module -avoid-version -shared
ricoscoreFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.ricoscoreFlag.txt \
	ricoscoreFlag.sln \
	ricoscoreFlag.vcxproj	\
	ricoscoreFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
