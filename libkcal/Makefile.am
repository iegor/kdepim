SUBDIRS = libical versit tests

INCLUDES = -I$(srcdir)/libical/src/libical \
  -I$(srcdir)/libical/src/libicalss -I./libical/src/libical \
  -I./libical/src/libicalss -I$(srcdir)/versit -I$(top_srcdir)/ \
  -I$(top_srcdir)/ktnef \
  -I$(top_srcdir)/libemailfunctions \
  $(all_includes)

lib_LTLIBRARIES   = libkcal.la
libkcal_la_LDFLAGS = $(all_libraries) -no-undefined -version-info 2:0:0
libkcal_la_LIBADD  = versit/libversit.la $(LIB_KIO) \
                     ./libical/src/libical/libical.la \
                     ./libical/src/libicalss/libicalss.la \
                     $(top_builddir)/ktnef/lib/libktnef.la \
                     ../libemailfunctions/libemailfunctions.la \
                     -lkresources -lkabc

libkcal_la_COMPILE_FIRST = ./libical/src/libical/ical.h ./libical/src/libicalss/icalss.h
libkcal_la_SOURCES = \
	incidencebase.cpp incidence.cpp journal.cpp todo.cpp event.cpp \
	freebusy.cpp attendee.cpp attachment.cpp recurrencerule.cpp recurrence.cpp alarm.cpp \
	customproperties.cpp calendar.cpp calendarlocal.cpp \
	calformat.cpp vcalformat.cpp icalformat.cpp icalformatimpl.cpp \
	incidenceformatter.cpp \
	vcaldrag.cpp icaldrag.cpp \
	exceptions.cpp \
	scheduler.cpp imipscheduler.cpp dummyscheduler.cpp \
	calfilter.cpp \
	person.cpp period.cpp duration.cpp \
	dndfactory.cpp \
	filestorage.cpp \
	compat.cpp \
	resourcecalendar.cpp resourcelocal.cpp resourcelocalconfig.cpp \
	resourcelocaldir.cpp resourcelocaldirconfig.cpp \
	resourcecached.cpp resourcecachedconfig.cpp \
	calendarresources.cpp \
	qtopiaformat.cpp \
	htmlexportsettings.kcfgc htmlexport.cpp calendarnull.cpp \
	freebusyurlstore.cpp \
	confirmsavedialog.cpp

libkcalincludedir = $(includedir)/libkcal
libkcalinclude_HEADERS = alarm.h attachment.h attendee.h calendar.h \
  calendarlocal.h calendarnull.h calendarresources.h calfilter.h calformat.h \
  calstorage.h customproperties.h dndfactory.h duration.h event.h \
  exceptions.h filestorage.h freebusy.h htmlexportsettings.h htmlexport.h icaldrag.h icalformat.h \
  incidencebase.h incidence.h incidenceformatter.h journal.h kcalversion.h listbase.h period.h person.h \
  qtopiaformat.h recurrencerule.h recurrence.h resourcecached.h resourcecalendar.h \
  resourcelocalconfig.h resourcelocaldirconfig.h resourcelocaldir.h \
  resourcelocal.h scheduler.h libkcal_export.h \
  todo.h vcaldrag.h vcalformat.h

kde_module_LTLIBRARIES = kcal_local.la kcal_localdir.la 

kcal_local_la_SOURCES = resourcelocal_plugin.cpp
kcal_local_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
kcal_local_la_LIBADD  = libkcal.la

kcal_localdir_la_SOURCES = resourcelocaldir_plugin.cpp
kcal_localdir_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
kcal_localdir_la_LIBADD  = libkcal.la

EXTRA_PROGRAMS = convertqtopia
convertqtopia_SOURCES = convertqtopia.cpp
convertqtopia_LDADD = libkcal.la

servicedir = $(kde_servicesdir)/kresources/kcal
service_DATA = local.desktop localdir.desktop

manager_DATA = kcal_manager.desktop
managerdir = $(kde_servicesdir)/kresources

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/libkcal.pot

DOXYGEN_EXCLUDE = libical
DOXYGEN_REFERENCES=kresources
include $(top_srcdir)/admin/Doxyfile.am
