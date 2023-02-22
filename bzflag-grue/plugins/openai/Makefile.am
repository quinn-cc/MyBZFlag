lib_LTLIBRARIES = openai.la

openai_la_SOURCES = openai.cpp
openai_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
openai_la_LDFLAGS = -module -avoid-version -shared
openai_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.openai.txt \
	openai.sln \
	openai.vcxproj	\
	openai.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
