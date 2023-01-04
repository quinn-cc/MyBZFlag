lib_LTLIBRARIES = shittyFlags.la

shittyFlags_la_SOURCES = shittyFlags.cpp
shittyFlags_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
shittyFlags_la_LDFLAGS = -module -avoid-version -shared
shittyFlags_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.shittyFlags.txt \
	shittyFlags.sln \
	shittyFlags.vcxproj	\
	shittyFlags.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
