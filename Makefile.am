## -*- Makefile -*-

SUBDIRS = src
if BUILD_DOCS
SUBDIRS += doc
endif
DIST_SUBDIRS = src doc

EXTRA_DIST = cthugha-L.lsm Makefile.pre  cthugha.ini.eg tga2jpeg \
	external/minimp3/LICENSE external/minimp3/minimp3.h external/minimp3/minimp3_ex.h

dist-hook:
	cp Makefile.pre $(distdir)/Makefile
	mkdir -p $(distdir)/resources
	ln -s ../../resources/img $(distdir)/resources/img
	ln -s ../../resources/map $(distdir)/resources/map
	ln -s ../../resources/obj $(distdir)/resources/obj
# ln -s ../precompiled $(distdir)/precompiled


## install image, palette, and object resources
install-data-local: ${noinst_PROGRAMS} ${noinst_DATA}
	${INSTALL} -d ${pkglibdir}/img
	${INSTALL} -d ${pkglibdir}/map
	${INSTALL} -d ${pkglibdir}/obj
	-cp resources/img/* ${pkglibdir}/img
	-cp resources/map/*.map ${pkglibdir}/map
	-cp resources/obj/*.obj ${pkglibdir}/obj
	-if test -d resources/map/png; then ${INSTALL} -d ${pkglibdir}/map/png; cp resources/map/png/* ${pkglibdir}/map/png; fi


AUTOMAKE_OPTIONS = foreign
