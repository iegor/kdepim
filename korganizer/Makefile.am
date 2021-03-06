SUBDIRS = printing . korgac pixmaps sounds plugins interfaces

INCLUDES = \
  -I$(srcdir)/interfaces \
  -I$(top_srcdir)/libemailfunctions \
  -I$(top_srcdir)/libkpimidentities \
  -I$(top_srcdir)/libkholidays \
  -I$(top_srcdir)/mimelib \
  -I$(top_srcdir)/certmanager/lib \
  -I$(top_srcdir)/korganizer/printing \
  -I$(top_srcdir) \
  -I$(top_builddir) \
  $(all_includes)

bin_PROGRAMS = korganizer

korganizer_LDFLAGS = $(all_libraries) $(KDE_RPATH)
korganizer_LDADD   = libkorganizer.la
korganizer_SOURCES = main.cpp korganizer.cpp koapp.cpp
korganizer_COMPILE_FIRST = koprefs_base.h


kde_module_LTLIBRARIES = kcm_korganizer.la libkorganizerpart.la

kcm_korganizer_la_SOURCES = koprefsdialog.cpp
kcm_korganizer_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_korganizer_la_LIBADD = libkorganizer.la $(top_builddir)/libkdepim/libkdepim.la \
  $(LIB_KDECORE)
kcm_korganizer_la_COMPILE_FIRST = koprefs_base.h 	kogroupwareprefspage.h

libkorganizerpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libkorganizerpart_la_LIBADD  = libkorganizer.la
libkorganizerpart_la_SOURCES = korganizer_part.cpp
libkorganizerpart_la_COMPILE_FIRST = publishdialog_base.h koprefs_base.h


check_PROGRAMS = korgplugins timezone

korgplugins_LDFLAGS = $(all_libraries) $(KDE_RPATH)
korgplugins_LDADD   = libkorganizer.la $(LIB_KSYCOCA)
korgplugins_SOURCES = korgplugins.cpp
korgplugins_COMPILE_FIRST = koprefs_base.h

timezone_LDFLAGS = $(all_libraries) $(KDE_RPATH)
timezone_LDADD   = libkorganizer.la $(LIB_KSYCOCA)
timezone_SOURCES = timezone.cpp
timezone_COMPILE_FIRST = koprefs_base.h

lib_LTLIBRARIES = libkorganizer_eventviewer.la \
                  libkorganizer_calendar.la libkorganizer.la

libkorganizer_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -version-info 1:0
libkorganizer_la_LIBADD  = \
	$(top_builddir)/libkcal/libkcal.la \
	$(top_builddir)/libkpimidentities/libkpimidentities.la \
	$(top_builddir)/libkdepim/libkdepim.la \
	libkorganizer_eventviewer.la \
	libkorganizer_calendar.la \
	printing/libkocorehelper.la \
	printing/libkorg_stdprinting.la \
	$(top_builddir)/kdgantt/libkdgantt.la \
	$(top_builddir)/libemailfunctions/libemailfunctions.la \
	$(top_builddir)/libkholidays/libkholidays.la \
	$(top_builddir)/libkmime/libkmime.la \
	$(LIB_KPARTS) $(LIB_KFILE) $(LIB_KNEWSTUFF) \
	-lkdeprint -lkabc -lkutils

libkorganizer_la_COMPILE_FIRST = \
	kogroupwareprefspage.ui \
	$(top_srcdir)/libkdepim/categoryselectdialog.h
libkorganizer_la_SOURCES = komessagebox.cpp \
	koagendaview.cpp koagenda.cpp koagendaitem.cpp \
	datenavigator.cpp kdatenavigator.cpp datenavigatorcontainer.cpp \
	datechecker.cpp \
	komonthview.cpp \
	searchdialog.cpp calendarview.cpp koviewmanager.cpp \
	kodialogmanager.cpp \
	archivedialog.cpp \
	kohelper.cpp \
	komailclient.cpp kotodoview.cpp kotodoviewitem.cpp \
	kolistview.cpp \
	koincidenceeditor.cpp koeventeditor.cpp kotodoeditor.cpp kojournaleditor.cpp\
	koprefs.cpp kowindowlist.cpp \
	koeditorgeneral.cpp \
	koeditorgeneralevent.cpp koeditorgeneraltodo.cpp koeditorgeneraljournal.cpp \
	koeditordetails.cpp koeditoralarms_base.ui koeditoralarms.cpp \
	koeditorrecurrence.cpp koeditorattachments.cpp \
	kogroupwareprefspage.ui koeventpopupmenu.cpp koeditorfreebusy.cpp \
	publishdialog_base.ui publishdialog.cpp \
	koeventview.cpp \
	korganizeriface.skel kcalendariface.skel \
	filtereditdialog.cpp filteredit_base.ui \
	kowhatsnextview.cpp kocounterdialog.cpp \
	kojournalview.cpp journalentry.cpp \
	kocore.cpp mailscheduler.cpp \
	kodaymatrix.cpp docprefs.cpp statusdialog.cpp\
	koglobals.cpp konewstuff.cpp \
	actionmanager.cpp resourceview.cpp \
	navigatorbar.cpp kogroupware.cpp \
	history.cpp \
	koprefs_base.kcfgc \
	koincidencetooltip.cpp aboutdata.cpp \
	importdialog.cpp \
	korganizerifaceimpl.cpp \
	freebusymanager.cpp freebusyurldialog.cpp \
	eventarchiver.cpp koidentitymanager.cpp \
	exportwebdialog.cpp kocorehelper.cpp incidencechanger.cpp \
	template_management_dialog_base.ui templatemanagementdialog.cpp \
	agendaview.cpp multiagendaview.cpp \
	timelineitem.cpp \
	kotimelineview.cpp \
	koattendeeeditor.cpp \
	timelabels.cpp


libkorganizer_eventviewer_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) \
                                       -no-undefined -version-info 1:0
libkorganizer_eventviewer_la_LIBADD  = $(LIB_KPARTS) \
                                       $(top_builddir)/libkcal/libkcal.la
libkorganizer_eventviewer_la_SOURCES = koeventviewer.cpp urihandler.cpp \
                                       kmailIface.stub koeventviewerdialog.cpp

libkorganizer_calendar_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) \
                                    -no-undefined -version-info 1:0
libkorganizer_calendar_la_LIBADD  = $(LIB_KPARTS) \
                                    $(top_builddir)/libkdepim/libkdepim.la \
                                    $(top_builddir)/libkcal/libkcal.la
libkorganizer_calendar_la_SOURCES = stdcalendar.cpp


kmailIface_DIR = $(top_srcdir)/kmail
kmailIface_DCOPIDLNG = true

METASOURCES = AUTO

noinst_HEADERS = komonthview.h \
	koagendaview.h koagenda.h \
	koagendaitem.h \
	kdatenavigator.h \
	calendarview.h \
	version.h \
	searchdialog.h \
	archivedialog.h \
	komailclient.h kotodoview.h \
	kolistview.h \
	koapp.h korganizer_part.h korganizer.h \
	koprefsdialog.h koeventeditor.h \
	kotodoeditor.h koprefs.h koeventviewer.h \
	kowindowlist.h kocounterdialog.h \
	koeditorgeneralevent.h koeditorgeneraltodo.h koeditordetails.h \
	koeditorrecurrence.h koeventviewerdialog.h koeventpopupmenu.h \
	publishdialog.h koeditorfreebusy.h \
	koeventview.h statusdialog.h customlistviewitem.h \
	kohelper.h \
	navigatorbar.h kogroupware.h \
	koincidencetooltip.h korganizerifaceimpl.h \
	exportwebdialog.h templatemanagementdialog.h

tip_DATA = tips
tipdir = $(kde_datadir)/korganizer

kde_bin_SCRIPTS = ical2vcal

rcdir = $(kde_datadir)/korganizer
rc_DATA = korganizerui.rc

partdir   = $(kde_datadir)/korganizer
part_DATA = korganizer_part.rc

kde_servicetypes_DATA = dcopcalendar.desktop

KDE_ICON = AUTO

META_INCLUDES = $(srcdir)/interfaces/korganizer

messages: rc.cpp
	$(PREPARETIPS) > tips.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` >> rc.cpp
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.h"` -o $(podir)/korganizer.pot
	rm -f tips.cpp

xdg_apps_DATA = korganizer.desktop korganizer-import.desktop

kde_kcfg_DATA = korganizer.kcfg

kde_services_DATA = korganizer_configmain.desktop \
                    korganizer_configtime.desktop \
                    korganizer_configviews.desktop \
                    korganizer_configcolors.desktop \
                    korganizer_configfonts.desktop \
                    korganizer_configplugins.desktop \
                    korganizer_configgroupscheduling.desktop \
                    korganizer_configfreebusy.desktop \
                    korganizer_configdesignerfields.desktop \
                    webcal.protocol

update_DATA = korganizer.upd
updatedir = $(kde_datadir)/kconf_update

DOXYGEN_REFERENCES=libkcal kdeui
include $(top_srcdir)/admin/Doxyfile.am

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(kde_servicesdir)
	$(mkinstalldirs) $(DESTDIR)$(kde_servicesdir)/korganizer
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_servicesdir)/korganizer_configgroupautomation.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_servicesdir)/korganizer/webexport.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_servicesdir)/korganizer/holidays.desktop
