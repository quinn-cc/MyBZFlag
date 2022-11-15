lib_LTLIBRARIES = gruesTurretDefenseMaster.la

gruesTurretDefenseMaster_la_SOURCES = gruesTurretDefenseMaster.cpp
gruesTurretDefenseMaster_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
gruesTurretDefenseMaster_la_LDFLAGS = -module -avoid-version -shared
gruesTurretDefenseMaster_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.gruesTurretDefenseMaster.txt \
	gruesTurretDefenseMaster.sln \
	gruesTurretDefenseMaster.vcxproj	\
	gruesTurretDefenseMaster.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
