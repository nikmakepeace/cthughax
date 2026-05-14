## -*- makefile -*-

## general source files
GENSRC = Option.cc OptionInt.cc CoreOption.cc CoreOptionEntry.cc	\
	SoundDeviceDSP.cc SoundDeviceFile.cc SoundDeviceFork.cc		\
	SoundDeviceNet.cc SoundDeviceRandom.cc				\
	Mixer.cc CDPlayer.cc imath.cc sound.cc	IniFiles.cc		\
	Interface.cc network.cc info_title_usage.cc disp-ncurses.cc	\
	SoundDevice.cc SoundServer.cc misc.cc DisplayDevice.cc		\
	AutoChanger.cc SoundAnalyze.cc  CthughaDisplay.cc 		\
	keymap.cc InterfaceCredits.cc InterfaceList.cc			\
	joystick.cc


DISPSRC = $(GENSRC)							\
	palettes.cc pcx.cc			\
	flames.cc waves.cc sound_tables.cc	\
	translate.cc 	 SoundProcess.cc \
	CthughaBuffer.cc initExitDisp.cc	\
	Flashlight.cc  InterfaceHelp.cc 


NONXSRC = nonx_keys.cc nonx_options.cc


cthugha_SOURCES = $(DISPSRC) $(NONXSRC) DisplayDeviceSvga.cc	\
	 svga_options.cc display.cc CthughaDisplaySVGA.cc	\
	nonGL_stubs.cc
# EXTRA_cthugha_SOURCES =	 getopt_long.c
cthugha_LDADD = ${N_LIBS} ${S_LIBS}  ${LIBOBJS}


xcthugha_SOURCES = $(DISPSRC) xwin_keys.cc xwin_options.cc display.cc		\
	DisplayDeviceX11.cc  CthughaDisplayX11.cc		\
	DisplayDeviceX11-Panel.cc nonGL_stubs.cc
# EXTRA_xcthugha_SOURCES = getopt_long.c
xcthugha_INCLUDES = ${X_CFLAGS}
xcthugha_LDADD = ${N_LIBS} ${X_LIBS}  ${LIBOBJS}

glcthugha_SOURCES = ${DISPSRC} GL_keys.cc GL_display.cc		\
	nonx_options.cc GL_options.cc DisplayDeviceGL.cc	\
	CthughaDisplayGL.cc GL_Light.cc	GL_Background.cc	\
	GL_Fly.cc
# EXTRA_glcthugha_SOURCES = getopt_long.c
glcthugha_INCLUDES = ${X_CFLAGS}
glcthugha_LDADD = ${N_LIBS} ${X_LIBS} ${GL_LIBS}  ${LIBOBJS}



cthugha_server_SOURCES = $(GENSRC) $(NONXSRC) serv_main.cc serv_display.cc	\
	serv_stubs.cc serv_options.cc nonGL_stubs.cc
# EXTRA_cthugha_server_SOURCES = getopt_long.c 
cthugha_server_LDADD = ${N_LIBS}  ${LIBOBJS}

tabheader_SOURCES = tabheader.cc

tabinfo_SOURCES = tabinfo.cc

#removed  getopt.h
## all the header files
noinst_HEADERS = 	\
	AutoChanger.h     SoundAnalyze.h    flames.h          pcx.h		\
        CDPlayer.h        SoundDevice.h  \
	CoreOption.h      SoundProcess.h    imath.h           tab_header.h	\
	CthughaBuffer.h   SoundServer.h     information.h     translate.h	\
	CthughaDisplay.h  cth_buffer.h      vroot.h		\
	DisplayDevice.h   cthugha.h         keys.h            waves.h		\
	Option.h          disp-sys.h        network.h         xcthugha.h	\
	Sound.h           display.h         options.h	      glcthugha.h	\
	Interface.h	  keymap.h	    joystick.h				\
										\
	initial_palettes.cc                 keys.cc options.cc			\
	default.keymap	  default.keymap.str



AM_CPPFLAGS = ${X_CFLAGS}


## programs installed normalls
bin_PROGRAMS = @TARGETS@
## programs that need special treatment (manual installation)
noinst_PROGRAMS = @TARGETS_SUID@

EXTRA_PROGRAMS = cthugha xcthugha glcthugha cthugha-server tabheader tabinfo

## for debugging
## display.s: 
##	$(COMPILE) display.cc -c -S -fverbose-asm -o display.s

##
## special installation for cthugha and xcthugha
## 
install-exec-local: @TARGETS_SUID@
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(bindir)
	-@list='$(noinst_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) -o root -m 4775 $$p $(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) -o root -m 4775 $$p $(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-local:
	$(NORMAL_UNINSTALL)
	list='$(noinst_PROGRAMS)'; for p in $$list; do \
	  rm -f $(bindir)/`echo $$p|sed '$(transform)'`; \
	done


#dist-hook:
#	cp -p $(bindir)/cthugha $(distdir)/cthugha
#	cp -p $(bindir)/xcthugha $(distdir)/xcthugha
#	cp -p $(bindir)/cthugha-server $(distdir)/cthugha-server
#	cp -p $(bindir)/tabinfo $(distdir)/tabinfo
#	cp -p $(bindir)/tabheader $(distdir)/tabheader




#
# create default keymap, so it can be included into the source
#
default.keymap.str: default.keymap
	sed -e "s/.*/\"&\",/" < default.keymap > default.keymap.str
