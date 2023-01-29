lib_LTLIBRARIES = shotgunFlag.la

shotgunFlag_la_SOURCES = shotgunFlag.cpp
shotgunFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
shotgunFlag_la_LDFLAGS = -module -avoid-version -shared
shotgunFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.shotgunFlag.txt \
	shotgunFlag.sln \
	shotgunFlag.vcxproj	\
	shotgunFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
