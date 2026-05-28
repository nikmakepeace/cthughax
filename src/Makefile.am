## -*- makefile -*-

## general source files
GENSRC = Option.cc OptionInt.cc CoreOption.cc CoreOptionEntry.cc	\
	Audio.cc AudioFrame.cc AudioRuntime.cc RuntimeFactory.cc		\
	PcmSourceFactory.cc AudioVisualBridge.cc			\
	Mixer.cc CDPlayer.cc imath.cc AudioSystem.cc IniFiles.cc		\
	Interface.cc info_title_usage.cc disp-ncurses.cc		\
	misc.cc DisplayDevice.cc				\
	AutoChanger.cc AudioAnalyzer.cc  CthughaDisplay.cc 		\
	CthughaFrameBuffer.cc Border.cc VisualPipeline.cc VisualDirector.cc	\
	keymap.cc InterfaceCredits.cc InterfaceList.cc			\
	joystick.cc


DISPSRC = $(GENSRC)							\
	palettes.cc pcx.cc			\
	flames.cc waves.cc sound_tables.cc	\
	translate.cc 	 AudioProcessor.cc \
	CthughaBuffer.cc initExitDisp.cc	\
	Flashlight.cc  InterfaceHelp.cc 


NONXSRC = nonx_keys.cc nonx_options.cc


cthugha_SOURCES = $(DISPSRC) $(NONXSRC) DisplayDeviceSvga.cc	\
	 svga_options.cc display.cc CthughaDisplaySVGA.cc	\
	nonGL_stubs.cc
# EXTRA_cthugha_SOURCES =	 getopt_long.c
cthugha_LDADD = ${N_LIBS} ${S_LIBS} ${PULSE_LIBS} ${LIBOBJS}


xcthugha_SOURCES = $(DISPSRC) xwin_keys.cc xwin_options.cc display.cc		\
	DisplayDeviceX11.cc  CthughaDisplayX11.cc		\
	DisplayDeviceX11-Panel.cc nonGL_stubs.cc
# EXTRA_xcthugha_SOURCES = getopt_long.c
xcthugha_INCLUDES = ${X_CFLAGS}
xcthugha_LDADD = ${N_LIBS} ${X_LIBS} -lz ${PULSE_LIBS} ${LIBOBJS}

glcthugha_SOURCES = ${DISPSRC} GL_keys.cc GL_display.cc		\
	nonx_options.cc GL_options.cc DisplayDeviceGL.cc	\
	CthughaDisplayGL.cc GL_Light.cc	GL_Background.cc	\
	GL_Fly.cc
# EXTRA_glcthugha_SOURCES = getopt_long.c
glcthugha_INCLUDES = ${X_CFLAGS}
glcthugha_LDADD = ${N_LIBS} ${X_LIBS} ${GL_LIBS} ${PULSE_LIBS} ${LIBOBJS}



tabheader_SOURCES = tabheader.cc

tabinfo_SOURCES = tabinfo.cc

#removed  getopt.h
## all the header files
noinst_HEADERS = 	\
	Audio.h           AudioFrame.h      AudioRuntime.h    AudioSystem.h     AudioOptions.h     AudioTypes.h       RuntimeFactory.h  PcmSourceFactory.h  AudioVisualBridge.h AutoChanger.h     AudioAnalyzer.h    CthughaFrameBuffer.h VisualPipeline.h VisualDirector.h flames.h          pcx.h		\
        CDPlayer.h        Mixer.h  \
	CoreOption.h      AudioProcessor.h    Border.h          Flashlight.h      imath.h           tab_header.h	\
	CthughaBuffer.h   information.h     translate.h	\
	CthughaDisplay.h  cth_buffer.h      vroot.h		\
	DisplayDevice.h   cthugha.h         keys.h            waves.h		\
	Option.h          disp-sys.h        xcthugha.h	\
	display.h         options.h	      glcthugha.h	\
	Interface.h	  keymap.h	    joystick.h				\
										\
	keys.cc options.cc			\
	default.keymap	  default.keymap.str



AM_CPPFLAGS = ${X_CFLAGS} ${PULSE_CFLAGS}


## programs installed normalls
bin_PROGRAMS = @TARGETS@
## programs that need special treatment (manual installation)
noinst_PROGRAMS = @TARGETS_SUID@

EXTRA_PROGRAMS = cthugha xcthugha glcthugha tabheader tabinfo

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
#	cp -p $(bindir)/tabinfo $(distdir)/tabinfo
#	cp -p $(bindir)/tabheader $(distdir)/tabheader




#
# create default keymap, so it can be included into the source
#
default.keymap.str: default.keymap
	sed -e "s/.*/\"&\",/" < default.keymap > default.keymap.str
