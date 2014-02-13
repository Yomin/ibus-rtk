
libexec_PROGRAMS = ibus-engine-rtk
ibus_engine_rtk_SOURCES = main.c engine.c engine.h lookup.c lookup.h
ibus_engine_rtk_CFLAGS = @IBUS_CFLAGS@ -DIBUS_RTK -DPKGDATADIR=\"${pkgdatadir}\"
ibus_engine_rtk_LDFLAGS = @IBUS_LIBS@

noinst_PROGRAMS = rtklookup
rtklookup_SOURCES = lookup.c lookup.h rtklookup.c

component_DATA = rtk.xml
componentdir = @datadir@/ibus/component

EXTRA_DIST = rtk.xml.in
CLEANFILES = rtk.xml

SUBST = " \
    s|%PACKAGE_VERSION%|@PACKAGE_VERSION@|g; \
    s|%PACKAGE_URL%|@PACKAGE_URL@|g; \
    s|%PACKAGE_BUGREPORT%|@PACKAGE_BUGREPORT@|g; \
    s|%libexecdir%|${libexecdir}|g; \
    s|%pkgdatadir%|${pkgdatadir}|g; \
"

rtk.xml: rtk.xml.in
	sed -e $(SUBST) $< >$@
