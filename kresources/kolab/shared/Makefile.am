INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/kresources/lib $(all_includes)

noinst_HEADERS = resourcekolabbase.h kolabbase.h subresource.h

noinst_LTLIBRARIES = libresourcekolabshared.la

libresourcekolabshared_la_SOURCES = \
	resourcekolabbase.cpp kmailconnection.cpp kolabbase.cpp \
        subresource.cpp \
	kmailconnection.skel kmailicalIface.stub
libresourcekolabshared_la_METASOURCES = AUTO
libresourcekolabshared_la_LIBADD = $(top_builddir)/libkcal/libkcal.la $(top_builddir)/libkdepim/libkdepim.la ../../lib/libkgroupwarebase.la
libresourcekolabshared_la_LDFLAGS = -no-undefined

kmailicalIface_DCOPIDLNG = true

kmailicalIface_DIR = $(top_srcdir)/kmail
