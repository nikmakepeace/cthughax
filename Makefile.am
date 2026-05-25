## -*- Makefile -*-

SUBDIRS = src tab
if BUILD_DOCS
SUBDIRS += doc
endif
DIST_SUBDIRS = src tab doc

EXTRA_DIST = cthugha-L.lsm Makefile.pre  cthugha.ini.eg tga2jpeg \
	external/minimp3/LICENSE external/minimp3/minimp3.h external/minimp3/minimp3_ex.h

dist-hook:
	cp Makefile.pre $(distdir)/Makefile
	ln -s ../pcx $(distdir)/pcx
	ln -s ../map $(distdir)/map
# ln -s ../precompiled $(distdir)/precompiled


## install the pcx and palette
install-data-local: ${noinst_PROGRAMS} ${noinst_DATA}
	${INSTALL} -d ${pkglibdir}/pcx
	${INSTALL} -d ${pkglibdir}/map
	-cp pcx/* ${pkglibdir}/pcx
	-cp map/*.map ${pkglibdir}/map
	-if test -d map/png; then ${INSTALL} -d ${pkglibdir}/map/png; cp map/png/* ${pkglibdir}/map/png; fi


AUTOMAKE_OPTIONS = foreign
