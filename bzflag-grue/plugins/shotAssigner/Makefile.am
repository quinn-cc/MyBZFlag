lib_LTLIBRARIES = shotAssigner.la

shotAssigner_la_SOURCES = shotAssigner.cpp
shotAssigner_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
shotAssigner_la_LDFLAGS = -module -avoid-version -shared
shotAssigner_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.shotAssigner.txt \
	shotAssigner.sln \
	shotAssigner.vcxproj	\
	shotAssigner.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
