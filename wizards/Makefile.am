METASOURCES = AUTO

INCLUDES = -I$(top_srcdir)/libkpimidentities -I$(top_srcdir)/libkcal \
           -I$(top_srcdir)/kresources/kolab/shared -I$(top_srcdir) \
           -I$(top_srcdir)/knotes \
           -I$(top_srcdir)/certmanager/lib \
           -I$(top_builddir)/kresources/groupwise \
           -I$(top_builddir)/kresources/lib \
           -I$(top_srcdir)/kresources/lib \
           $(all_includes)

bin_PROGRAMS = groupwarewizard egroupwarewizard sloxwizard kolabwizard \
               groupwisewizard exchangewizard scalixwizard

kde_module_LTLIBRARIES = libegroupwarewizard.la libsloxwizard.la \
                         libkolabwizard.la libgroupwisewizard.la \
                         libexchangewizard.la libscalixwizard.la


groupwarewizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
groupwarewizard_LDADD = libegroupwarewizard.la libsloxwizard.la $(LIB_KDEUI) \
                        libkolabwizard.la libgroupwisewizard.la \
                        libexchangewizard.la
groupwarewizard_SOURCES = groupwarewizard.cpp main.cpp overviewpage.cpp

# Kolab
libkolabwizard_la_LDFLAGS = -avoid-version -no-undefined $(all_libraries)
libkolabwizard_la_LIBADD = $(top_builddir)/kresources/kolab/kcal/libkcalkolab.la \
                           $(top_builddir)/kresources/kolab/kabc/libkabckolab.la \
                           $(top_builddir)/kresources/kolab/knotes/libknoteskolab.la \
                           $(top_builddir)/libkcal/libkcal.la \
                           $(top_builddir)/libkdepim/libkdepim.la \
                           $(top_builddir)/libkpimidentities/libkpimidentities.la

libkolabwizard_la_SOURCES = kolabwizard.cpp kmailchanges.cpp kolabconfig.kcfgc \
  kolabkmailchanges.cpp

kolabwizard_LDADD = libkolabwizard.la $(LIB_KDEUI)
kolabwizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kolabwizard_SOURCES = kolabmain.cpp

# eGroupware
libegroupwarewizard_la_LDFLAGS = -avoid-version -no-undefined $(all_libraries)
libegroupwarewizard_la_SOURCES = egroupwarewizard.cpp egroupwareconfig.kcfgc
libegroupwarewizard_la_LIBADD = $(top_builddir)/kresources/egroupware/libkabc_xmlrpc.la \
                                $(top_builddir)/kresources/egroupware/libkcal_xmlrpc.la \
                                $(top_builddir)/kresources/egroupware/libknotes_xmlrpc.la \
                                $(top_builddir)/libkcal/libkcal.la \
                                $(top_builddir)/libkdepim/libkdepim.la \
                                $(LIB_KDEUI)

egroupwarewizard_LDADD = libegroupwarewizard.la
egroupwarewizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
egroupwarewizard_SOURCES = egroupwaremain.cpp

# SLOX
libsloxwizard_la_LDFLAGS = -avoid-version -no-undefined $(all_libraries)
libsloxwizard_la_SOURCES = sloxwizard.cpp sloxconfig.kcfgc
libsloxwizard_la_LIBADD = $(top_builddir)/kresources/slox/libkcal_slox.la \
                          $(top_builddir)/kresources/slox/libkabc_slox.la \
                          $(top_builddir)/libkdepim/libkdepim.la

sloxwizard_LDADD = libsloxwizard.la
sloxwizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
sloxwizard_SOURCES = sloxmain.cpp

# GroupWise
libgroupwisewizard_la_LDFLAGS = -avoid-version -no-undefined $(all_libraries)
libgroupwisewizard_la_SOURCES = groupwisewizard.cpp groupwiseconfig.kcfgc \
  kmailchanges.cpp
libgroupwisewizard_la_LIBADD = $(top_builddir)/kresources/groupwise/libkcal_groupwise.la \
                          $(top_builddir)/kresources/groupwise/libkabc_groupwise.la \
                          $(top_builddir)/libkdepim/libkdepim.la \
                          $(top_builddir)/libkpimidentities/libkpimidentities.la
libgroupwisewizard_la_COMPILE_FIRST = $(top_builddir)/kresources/groupwise/kabc_groupwiseprefs.h \
                                      $(top_builddir)/kresources/groupwise/kcal_groupwiseprefsbase.h


groupwisewizard_LDADD = libgroupwisewizard.la
groupwisewizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
groupwisewizard_SOURCES = groupwisemain.cpp

# Exchange
libexchangewizard_la_LDFLAGS = -avoid-version -no-undefined $(all_libraries)
libexchangewizard_la_SOURCES = exchangewizard.cpp
libexchangewizard_la_LIBADD = $(top_builddir)/kresources/newexchange/libkcal_newexchange.la \
                              $(top_builddir)/kresources/newexchange/libkabc_newexchange.la \
                              $(top_builddir)/libkdepim/libkdepim.la

exchangewizard_LDADD = libexchangewizard.la
exchangewizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
exchangewizard_SOURCES = exchangemain.cpp

# Scalix
libscalixwizard_la_LDFLAGS = -avoid-version -no-undefined $(all_libraries)
libscalixwizard_la_LIBADD = $(top_builddir)/kresources/scalix/kcal/libkcalscalix.la \
                           $(top_builddir)/kresources/scalix/kabc/libkabcscalix.la \
                           $(top_builddir)/libkcal/libkcal.la \
                           $(top_builddir)/libkdepim/libkdepim.la \
                           $(top_builddir)/libkpimidentities/libkpimidentities.la

libscalixwizard_la_SOURCES = scalixwizard.cpp kmailchanges.cpp scalixconfig.kcfgc \
  scalixkmailchanges.cpp

scalixwizard_LDADD = libscalixwizard.la $(LIB_KDEUI)
scalixwizard_LDFLAGS = $(all_libraries) $(KDE_RPATH)
scalixwizard_SOURCES = scalixmain.cpp

noinst_HEADERS = egroupwarewizard.h kmailchanges.h kolabwizard.h sloxwizard.h \
                 groupwisewizard.h exchangewizard.h

kde_kcfg_DATA = egroupware.kcfg slox.kcfg kolab.kcfg groupwise.kcfg scalix.kcfg

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kdepimwizards.pot

egroupwarewizard.lo: ../kresources/egroupware/kabc_egroupwareprefs.h ../kresources/egroupware/kcal_egroupwareprefs.h ../kresources/egroupware/knotes_egroupwareprefs.h
exchangewizard.lo: ../kresources/lib/kresources_groupwareprefs.h
sloxwizard.lo: ../kresources/slox/kabcsloxprefs.h ../kresources/slox/kcalsloxprefs.h

xdg_apps_DATA = groupwarewizard.desktop
