## -*- makefile -*-

## general source files
GENSRC = Option.cc OptionInt.cc CoreOption.cc CoreOptionEntry.cc	\
	Audio.cc AudioFrame.cc AudioRuntime.cc Settings.cc RuntimeFactory.cc Scene.cc Screenshot.cc		\
	PcmSourceFactory.cc AudioVisualBridge.cc			\
	Mixer.cc CDPlayer.cc imath.cc AudioSystem.cc IniFiles.cc		\
	Interface.cc info_title_usage.cc disp-ncurses.cc		\
	misc.cc DisplayDevice.cc				\
	AutoChanger.cc AudioAnalyzer.cc  CthughaDisplay.cc 		\
	Border.cc ColorPalette.cc Image.cc VisualPipeline.cc VisualPipelineSequence.cc	\
	PipelineStageModules.cc VisualPipelineFactory.cc VisualDirector.cc	\
	keymap.cc InterfaceCredits.cc InterfaceList.cc


DISPSRC = $(GENSRC)							\
	palettes.cc pcx.cc png.cc			\
	Flame.cc flames.cc FramePalette.cc PaletteTransition.cc Translate.cc TranslateGenerator.cc Wave.cc waves.cc sound_tables.cc	\
	translate.cc 	 AudioProcessor.cc \
	CthughaBuffer.cc initExitDisp.cc	\
	Flashlight.cc  InterfaceHelp.cc 


xcthugha_SOURCES = $(DISPSRC) xwin_keys.cc xwin_options.cc display.cc		\
	DisplayDeviceX11.cc  CthughaDisplayX11.cc		\
	DisplayDeviceX11-Panel.cc
# EXTRA_xcthugha_SOURCES = getopt_long.c
xcthugha_INCLUDES = ${X_CFLAGS}
xcthugha_LDADD = ${N_LIBS} ${X_LIBS} -lz ${PULSE_LIBS} ${LIBOBJS}


#removed  getopt.h
## all the header files
noinst_HEADERS = 	\
	Audio.h           AudioFrame.h      AudioRuntime.h    AudioSystem.h     AudioOptions.h     AudioTypes.h       Settings.h RuntimeFactory.h  Scene.h Screenshot.h PcmSourceFactory.h  AudioVisualBridge.h AutoChanger.h     AudioAnalyzer.h    VisualPipeline.h VisualPipelineSequence.h VisualPipelineFactory.h PipelineStageModules.h VisualDirector.h Flame.h flames.h FramePalette.h PaletteTransition.h Wave.h Image.h          pcx.h png.h		\
        CDPlayer.h        Mixer.h  \
	CoreOption.h      AudioProcessor.h    Border.h          ColorPalette.h Flashlight.h      imath.h	\
	CthughaBuffer.h   information.h     Translate.h TranslateGenerator.h TranslationTable.h translate.h	\
	CthughaDisplay.h  cth_buffer.h      vroot.h		\
	DisplayDevice.h   cthugha.h         keys.h            waves.h		\
	Option.h          disp-sys.h        xcthugha.h	\
	display.h         options.h	      \
	Interface.h	  keymap.h	    				\
										\
	keys.cc options.cc			\
	default.keymap	  default.keymap.str



AM_CPPFLAGS = ${X_CFLAGS} ${PULSE_CFLAGS}


## programs installed normalls
bin_PROGRAMS = @TARGETS@
## programs that need special treatment (manual installation)
noinst_PROGRAMS = @TARGETS_SUID@

EXTRA_PROGRAMS = xcthugha

## for debugging
## display.s: 
##	$(COMPILE) display.cc -c -S -fverbose-asm -o display.s

##
## special installation for targets that still need setuid handling
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
#	cp -p $(bindir)/xcthugha $(distdir)/xcthugha




#
# create default keymap, so it can be included into the source
#
default.keymap.str: default.keymap
	sed -e "s/.*/\"&\",/" < default.keymap > default.keymap.str
