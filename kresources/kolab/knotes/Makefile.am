METASOURCES = AUTO

INCLUDES = -I$(top_srcdir)/kresources/kolab/shared \
	-I$(top_srcdir) -I$(top_srcdir)/knotes -I$(top_builddir)/libkdepim $(all_includes)

# The kolab wizard links to this library too
lib_LTLIBRARIES = libknoteskolab.la

libknoteskolab_la_SOURCES = resourcekolab.cpp note.cpp
libknoteskolab_la_LDFLAGS = $(all_libraries) -no-undefined
libknoteskolab_la_LIBADD = \
	$(top_builddir)/kresources/kolab/shared/libresourcekolabshared.la \
	$(top_builddir)/knotes/libknotesresources.la \
	$(top_builddir)/libkcal/libkcal.la \
	-lkresources -lkdeprint

kde_module_LTLIBRARIES = knotes_kolab.la

knotes_kolab_la_SOURCES = resourcekolab_plugin.cpp
knotes_kolab_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
knotes_kolab_la_LIBADD = libknoteskolab.la

servicedir = $(kde_servicesdir)/kresources/knotes
service_DATA = kolabresource.desktop

install-data-local: $(srcdir)/../uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(servicedir)
	$(INSTALL_DATA) $(srcdir)/../uninstall.desktop $(DESTDIR)$(servicedir)/imap.desktop

