lib_LTLIBRARIES = observerHandler.la

observerHandler_la_SOURCES = observerHandler.cpp
observerHandler_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
observerHandler_la_LDFLAGS = -module -avoid-version -shared
observerHandler_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.observerHandler.txt \
	observerHandler.sln \
	observerHandler.vcxproj	\
	observerHandler.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
