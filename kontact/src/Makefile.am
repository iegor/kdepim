SUBDIRS = about

INCLUDES = -I$(top_srcdir)/kontact/interfaces -I$(top_srcdir) $(all_includes)

lib_LTLIBRARIES = libkontact.la
libkontact_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -version-info 1:0
libkontact_la_LIBADD = $(LIB_KDECORE)
libkontact_la_SOURCES = prefs.kcfgc

bin_PROGRAMS = kontact

kontact_METASOURCES = AUTO
kontact_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kontact_LDADD = $(top_builddir)/libkdepim/libkdepim.la \
  $(top_builddir)/kontact/interfaces/libkpinterfaces.la libkontact.la \
  $(LIB_KPARTS) $(LIB_KUTILS) $(LIB_KHTML)
kontact_SOURCES = main.cpp mainwindow.cpp sidepanebase.cpp \
  iconsidepane.cpp aboutdialog.cpp profilemanager.cpp profiledialog.cpp \
  kontactiface.skel
kontact_COMPILE_FIRST = prefs.h

kde_module_LTLIBRARIES = kcm_kontact.la

kcm_kontact_la_SOURCES = kcmkontact.cpp
kcm_kontact_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_kontact_la_LIBADD = libkontact.la $(top_builddir)/libkdepim/libkdepim.la
kcm_kontact_la_COMPILE_FIRST = prefs.h


rcdir = $(kde_datadir)/kontact
rc_DATA = kontactui.rc

xdg_apps_DATA = Kontact.desktop kontactdcop.desktop

kde_kcfg_DATA = kontact.kcfg

kde_services_DATA = kontactconfig.desktop

kontactsetdlgdir = $(kde_datadir)/kontact
kontactsetdlg_DATA = kontact.setdlg

KDE_ICON = AUTO
