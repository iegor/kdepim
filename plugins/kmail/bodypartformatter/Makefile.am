KDE_OPTIONS = nofinal
KDE_CXXFLAGS = $(USE_RTTI)

INCLUDES = -I$(top_srcdir)/kmail \
           -I$(top_srcdir)/libkdepim \
           -I$(top_srcdir)/libkmime \
           -I$(top_srcdir)/mimelib \
           -I$(top_srcdir)/libemailfunctions \
           -I$(top_srcdir) \
           -I$(top_builddir)/libkdepim \
           -I$(top_srcdir)/korganizer \
           $(all_includes)

kde_module_LTLIBRARIES = libkmail_bodypartformatter_text_vcard.la \
                         libkmail_bodypartformatter_text_calendar.la \
                         libkmail_bodypartformatter_text_xdiff.la

libkmail_bodypartformatter_text_vcard_la_SOURCES = text_vcard.cpp
libkmail_bodypartformatter_text_vcard_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
libkmail_bodypartformatter_text_vcard_la_LIBADD  = $(LIB_QT) $(LIB_KABC) \
                                                   ../../../libkdepim/libkdepim.la \
                                                   ../../../libkmime/libkmime.la \
                                                   ../../../libemailfunctions/libemailfunctions.la

libkmail_bodypartformatter_text_calendar_la_SOURCES = \
	text_calendar.cpp \
	delegateselector.cpp \
	attendeeselector.cpp \
	ui_attendeeselector.ui \
	kcalendariface.stub
libkmail_bodypartformatter_text_calendar_la_LDFLAGS = \
  $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
libkmail_bodypartformatter_text_calendar_la_LIBADD  = $(LIB_QT) $(LIB_KABC) \
  $(top_builddir)/libkcal/libkcal.la $(top_builddir)/kmail/libkmailprivate.la

kcalendariface_DIR = $(top_srcdir)/korganizer

libkmail_bodypartformatter_text_xdiff_la_SOURCES = text_xdiff.cpp
libkmail_bodypartformatter_text_xdiff_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libkmail_bodypartformatter_text_xdiff_la_LIBADD  = $(LIB_QT)


linkdir = $(kde_datadir)/kmail/plugins/bodypartformatter
link_DATA = text_vcard.desktop text_calendar.desktop text_xdiff.desktop

METASOURCES = AUTO

messages:
	$(EXTRACTRC) ui_attendeeselector.ui >> ui_attendeeselector.rc.cpp
	$(XGETTEXT) text_vcard.cpp -o $(podir)/kmail_text_vcard_plugin.pot
	$(XGETTEXT) text_calendar.cpp delegateselector.cpp attendeeselector.cpp ui_attendeeselector.rc.cpp -o $(podir)/kmail_text_calendar_plugin.pot
	$(XGETTEXT) text_xdiff.cpp -o $(podir)/kmail_text_xdiff_plugin.pot
