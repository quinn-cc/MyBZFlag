lib_LTLIBRARIES = scarwallServerMessages.la

scarwallServerMessages_la_SOURCES = scarwallServerMessages.cpp
scarwallServerMessages_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
scarwallServerMessages_la_LDFLAGS = -module -avoid-version -shared
scarwallServerMessages_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.scarwallServerMessages.txt \
	scarwallServerMessages.sln \
	scarwallServerMessages.vcxproj	\
	scarwallServerMessages.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
