KDE_CXXFLAGS = $(USE_THREADS)

INCLUDES=	-I$(top_srcdir)/libkmime \
		-I$(top_srcdir)/libkdepim \
		-I$(top_srcdir)/libkpgp \
		-I$(top_srcdir)/libemailfunctions \
		-I$(top_srcdir) \
		 $(all_includes)

lib_LTLIBRARIES = libknodecommon.la

kde_module_LTLIBRARIES = kcm_knode.la libknodepart.la
libknodepart_la_LDFLAGS = -module -avoid-version -no-undefined $(all_libraries) $(KDE_RPATH) $(KDE_PLUGIN)
libknodepart_la_LIBADD	= libknodecommon.la

SUBDIRS = pics filters

bin_PROGRAMS = knode

knode_LDFLAGS = $(all_libraries) $(KDE_RPATH)
knode_LDADD   = libknodecommon.la

kcm_knode_la_SOURCES = knconfigpages.cpp
kcm_knode_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_knode_la_LIBADD = libknodecommon.la $(LIB_KDECORE)
knconfigpages.lo: smtpaccountwidget_base.h

libknodecommon_la_LDFLAGS = -version-info 3:0:0 -no-undefined $(all_libraries)
libknodecommon_la_LIBADD = $(top_builddir)/libkmime/libkmime.la $(top_builddir)/libkpgp/libkpgp.la $(top_builddir)/libkdepim/libkdepim.la $(LIB_KSPELL) $(LIB_KABC) $(LIB_KFILE) $(LIB_KUTILS) $(LIBRESOLV) $(LIB_KHTML)
libknodecommon_la_SOURCES =   knconfigmanager.cpp \
                  knconfig.cpp \
                  knconfigwidgets.cpp \
                  \
                  knnetaccess.cpp \
                  knprotocolclient.cpp \
                  knnntpclient.cpp \
                  knjobdata.cpp \
                  \
                  knaccountmanager.cpp \
                  kncollection.cpp \
                  kncollectionviewitem.cpp \
                  knserverinfo.cpp \
                  knnntpaccount.cpp \
                  \
                  kngroupmanager.cpp \
                  knarticlecollection.cpp \
                  kngroup.cpp \
                  kngroupbrowser.cpp \
                  kngroupselectdialog.cpp \
                  kngroupdialog.cpp \
                  kngrouppropdlg.cpp \
                  \
                  knfoldermanager.cpp \
                  knfolder.cpp \
                  \
                  knmemorymanager.cpp \
                  kncleanup.cpp \
                  knconvert.cpp \
                  \
                  knarticlemanager.cpp \
                  knarticle.cpp \
                  kndisplayedheader.cpp \
                  knsourceviewwindow.cpp \
                  knarticlewindow.cpp \
                  knhdrviewitem.cpp \
                  \
                  kncomposer.cpp \
                  knarticlefactory.cpp \
                  \
                  knfiltermanager.cpp \
                  knstatusfilter.cpp \
                  knstringfilter.cpp \
                  knrangefilter.cpp \
                  knarticlefilter.cpp \
                  knfilterconfigwidget.cpp \
                  knfilterdialog.cpp \
                  knsearchdialog.cpp \
                  \
                  utilities.cpp \
                  \
                  knscoring.cpp \
                  \
                  knwidgets.cpp \
                  headerview.cpp \
		  knmainwidget.cpp    \
		  aboutdata.cpp \
		  knglobals.cpp \
		  knodecomposeriface.skel \
		  knodeiface.skel \
		  kncollectionview.cpp \
                  articlewidget.cpp \
                  csshelper.cpp \
                  smtpaccountwidget_base.ui
libknodecommon_la_COMPILE_FIRST = smtpaccountwidget_base.h

knode_SOURCES = knode.cpp knapplication.cpp main.cpp

libknodepart_la_SOURCES = knode_part.cpp


noinst_HEADERS =  knconfigmanager.h \
                  knconfig.h \
                  knconfigwidgets.h \
                  \
                  knnetaccess.h \
                  knprotocolclient.h \
                  knnntpclient.h \
                  knjobdata.h \
                  \
                  knaccountmanager.h \
                  kncollection.h \
                  kncollectionviewitem.h \
                  knserverinfo.h \
                  knnntpaccount.h \
                  \
                  kngroupmanager.h \
                  knarticlecollection.h \
                  kngroup.h \
                  kngroupbrowser.h \
                  kngroupselectdialog.h \
                  kngroupdialog.h \
                  kngrouppropdlg.h \
                  \
                  knfoldermanager.h \
                  knfolder.h \
                  \
                  knmemorymanager.h \
                  kncleanup.h \
                  knconvert.h \
                  \
                  knarticlemanager.h \
                  knarticle.h \
                  kndisplayedheader.h \
                  knsourceviewwindow.h \
                  knarticlewindow.h \
                  knhdrviewitem.h \
                  \
                  kncomposer.h \
                  knarticlefactory.h \
                  \
                  knfiltermanager.h \
                  knstatusfilter.h \
                  knstringfilter.h \
                  knrangefilter.h \
                  knarticlefilter.h \
                  knfilterconfigwidget.h \
                  knfilterdialog.h \
                  knsearchdialog.h \
                  \
                  utilities.h \
                  \
                  knscoring.h \
                  \
                  knwidgets.h \
                  headerview.h \
                  knode.h \
                  knapplication.h \
                  resource.h \
                  knglobals.h \
		  aboutdata.h \
		  knmainwidget.h \
		  kncollectionview.h \
                  articlewidget.h \
                  csshelper.h


knode_METASOURCES = AUTO

xdg_apps_DATA = KNode.desktop

rcdir = $(kde_datadir)/knode
rc_DATA = headers.rc knodeui.rc knreaderui.rc kncomposerui.rc

knewsprotocol_DATA = knewsservice.protocol
knewsprotocoldir = $(kde_servicesdir)

kde_services_DATA = knode_config_identity.desktop knode_config_accounts.desktop knode_config_appearance.desktop knode_config_read_news.desktop knode_config_post_news.desktop knode_config_privacy.desktop knode_config_cleanup.desktop

KDE_ICON = AUTO

EXTRA_DIST = KNode.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/knode.pot
