INCLUDES= $(PISOCK_INCLUDE) -I$(top_srcdir)/kpilot/lib $(all_includes)

kde_module_LTLIBRARIES = conduit_address.la

conduit_address_la_SOURCES = \
	resolutionDialog_base.ui \
	kaddressbookConduit.ui \
	abbrowserSettings.kcfgc \
	resolutionDialog.cc \
	abbrowser-factory.cc \
	abbrowser-setup.cc \
	kabcRecord.cc \
	abbrowser-conduit.cc
conduit_address_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
conduit_address_la_LIBADD = ../../lib/libkpilot.la -lkabc -lkabc_file

servicedir = $(kde_servicesdir)
service_DATA = abbrowser_conduit.desktop
kde_kcfg_DATA = abbrowserconduit.kcfg

kabcustompagedir = $(kde_datadir)/kaddressbook/contacteditorpages
kabcustompage_DATA = KPilotCustomFieldEditor.ui

METASOURCES = AUTO
