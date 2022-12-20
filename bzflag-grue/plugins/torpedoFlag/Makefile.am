lib_LTLIBRARIES = torpedoFlag.la

torpedoFlag_la_SOURCES = torpedoFlag.cpp
torpedoFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
torpedoFlag_la_LDFLAGS = -module -avoid-version -shared
torpedoFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.torpedoFlag.txt \
	torpedoFlag.sln \
	torpedoFlag.vcxproj	\
	torpedoFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
