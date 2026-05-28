#include "cthugha.h"
#include "options.h"
#include "display.h"
#include "translate.h"
#include "information.h"
#include "display.h"
#include "Sound.h"
#include "CDPlayer.h"
#include "keys.h"
#include "waves.h"
#include "cth_buffer.h"
#include "AutoChanger.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "AudioProcessor.h"
#include "Border.h"
#include "Flashlight.h"
#include "DisplayDevice.h"
#include "AudioAnalyzer.h"
#include "VisualDirector.h"
#ifdef CTH_XWIN
#include "xcthugha.h"
#endif
#include "imath.h"
#ifdef CTH_GL
#include "glcthugha.h"
#endif
#include "keymap.h"
#include "joystick.h"

#include <unistd.h>
#include <ctype.h>

enum option_nr {
    opt_verbose = 16000,
    opt_play,
    opt_snd_fragments,
    opt_mixer,
    opt_msg_time,
    opt_prt_file,
    opt_fire_level,
    opt_min_noise,
    opt_listen,
    opt_wave_scale,
    opt_position,
    opt_sound_method,
    opt_sound_buffer,
    opt_sound_format,
    opt_dev_dsp,
    opt_dev_mixer,
    opt_dev_cd,
    opt_fifo,
    opt_border,
    opt_test,
    opt_zoom,
    opt_maxfps,
    opt_flashlight,
    opt_no_flashlight,
    opt_font,
    opt_mesh_size,
    opt_texture_quality,
    opt_silent,
    opt_no_silent,
    opt_dsp_sync,
    opt_no_dsp_sync,
    opt_no_snd_buffer,
    opt_light,
    opt_no_light,
    opt_hints,
    opt_dither,
    opt_no_dither,
    opt_background,
    opt_no_background,
    opt_fly,
    opt_no_fly,
    opt_keymap,
    opt_shade,
    opt_no_shade,
    opt_palette_smoothing,
    opt_no_palette_smoothing,
    opt_palette_set,
    opt_pulse_server,
    opt_pulse_latency_ms,
    opt_audio_output_dump,
    opt_ini_file,
};

struct option long_options[] = {
    // selecting sound device options
    { "no-sound", 0, 0, 'x' },

    { "play", 1, 0, opt_play }, { "silent", 0, 0, opt_silent },
    { "no-silent", 0, 0, opt_no_silent },

    // general sound options
    { "rate", 1, 0, 'v' }, { "stereo", 0, 0, '2' }, { "no-stereo", 0, 0, '1' },
    { "snd-format", 1, 0, opt_sound_format },
    { "pulse-server", 1, 0, opt_pulse_server },
    { "pulse-latency-ms", 1, 0, opt_pulse_latency_ms },
    { "audio-output-dump", 1, 0, opt_audio_output_dump },

// DSP options
#if WITH_DSP == 1
    { "snd-method", 1, 0, opt_sound_method },
    { "snd-sync", 0, 0, opt_dsp_sync }, { "no-snd-sync", 0, 0, opt_no_dsp_sync },
    { "snd-fragments", 1, 0, opt_snd_fragments },
    { "dev-dsp", 1, 0, opt_dev_dsp },
#endif

    // Play/exec options
    { "fifo", 1, 0, opt_fifo }, { "snd-buffer", 1, 0, opt_sound_buffer },
    { "no-snd-buffer", 0, 0, opt_no_snd_buffer }, { "loop", 0, &soundPlayLoop, 1 },
    { "no-loop", 0, &soundPlayLoop, 0 },

// CD options
#if WITH_CDROM == 1
    { "dev-cd", 1, 0, opt_dev_cd }, { "cd-stop", 0, &cd_stop_on_exit.value, 1 },
    { "no-cd-stop", 0, &cd_stop_on_exit.value, 0 }, { "cd-random", 0, &cd_randomplay.value, 1 },
    { "no-cd-random", 0, &cd_randomplay.value, 0 },
    { "cd-loop", 0, &cd_loop.value, 1 }, { "no-cd-loop", 0, &cd_loop.value, 0 },
    { "cd-eject", 0, &cd_eject_on_end.value, 1 },
    { "no-cd-eject", 0, &cd_eject_on_end.value, 0 }, { "track", 1, 0, 'c' },
#endif

// Mixer options
#if WITH_MIXER == 1
    { "dev-mixer", 1, 0, opt_dev_mixer }, { "mixer", 1, 0, opt_mixer }, { "line", 1, 0, 'L' },
    { "mic", 1, 0, 'M' }, { "cd", 1, 0, 'C' },
#endif

// auto changer options
    { "lock", 0, &lock.value, 1 }, { "no-lock", 0, &lock.value, 0 },
    { "little", 0, &change_little.value, 1 }, { "no-little", 0, &change_little.value, 0 },
    { "min-time", 1, 0, 'T' }, { "random-time", 1, 0, 'R' },
    { "msg-time", 1, 0, opt_msg_time }, { "quiet-time", 1, 0, 'Q' },
    { "quiet-file", 1, 0, 'q' }, { "min-noise", 1, 0, opt_min_noise },
    { "fire-level", 1, 0, opt_fire_level },

// Core Options
    { "flashlight", 0, 0, opt_flashlight }, { "no-flashlight", 0, 0, opt_no_flashlight },
    { "sound-processing", 1, 0, 'm' }, { "flame", 1, 0, 'f' }, { "translation", 1, 0, 't' },
    { "light", 1, 0, opt_light }, { "no-light", 1, 0, opt_no_light },

// buffer options
    { "buff-size", 1, 0, 'S' }, { "wave", 1, 0, 'w' },
    { "wave-scale", 1, 0, opt_wave_scale }, { "object", 1, 0, 'o' },
    { "no-object", 0, &use_objects.value, 0 },
    { "use-object", 0, &use_objects.value, 1 }, { "trans", 0, &use_translates.value, 1 },
    { "no-trans", 0, &use_translates.value, 0 }, { "stretch", 0, &trans_stretch.value, 1 },
    { "no-stretch", 0, &trans_stretch.value, 0 },
    { "load-on-demand", 0, &transLoadOnDemand.value, 1 },
    { "no-load-on-demand", 0, &transLoadOnDemand.value, 0 },
    { "load-late", 0, &transLoadLate.value, 1 }, { "no-load-late", 0, &transLoadLate.value, 0 },
    { "table", 1, 0, 'a' }, { "border", 1, 0, opt_border },

// display options
    { "disp-mode", 1, 0, 'D' }, { "palette", 1, 0, 'p' }, { "display", 1, 0, 'd' },
    { "pcx", 1, 0, 'P' }, { "use-pcx", 0, &display_use_pcx, 1 },
    { "no-pcx", 0, &display_use_pcx, 0 },
    { "palette-smoothing", 1, 0, opt_palette_smoothing },
    { "no-palette-smoothing", 0, 0, opt_no_palette_smoothing },
    { "palette-set", 1, 0, opt_palette_set },
    { "test", 0, 0, opt_test }, { "max-fps", 1, 0, opt_maxfps },
    { "zoom", 1, 0, opt_zoom },

// SVGA options
    { "sync", 0, &display_syncwait, 1 }, { "no-sync", 0, &display_syncwait, 0 },
#ifndef CTH_GL
    { "disp-direct", 0, &display_direct, 1 }, { "no-disp-direct", 0, &display_direct, 0 },
#endif

// X11 options
#ifdef CTH_XWIN
    { "mit-shm", 0, &display_mit_shm, 1 }, { "no-mit-shm", 0, &display_mit_shm, 0 },
    { "root", 0, &display_on_root, 1 }, { "no-root", 0, &display_on_root, 0 },
    { "install", 0, &private_cmap, 1 }, { "no-install", 0, &private_cmap, 0 },
    { "override-redirect", 0, &display_override_redirect, 1 },
    { "no-override-redirect", 0, &display_override_redirect, 0 },
    { "no-decorate", 0, &display_override_redirect, 1 },
    { "decorate", 0, &display_override_redirect, 0 }, { "full-screen", 0, &full_screen, 1 },
    { "no-full-screen", 0, &full_screen, 0 }, { "panel", 0, &xcth_panel, 1 },
    { "no-panel", 0, &xcth_panel, 0 }, { "position", 1, 0, opt_position },
    { "text-on-term", 0, &DisplayDevice::text_on_term, 1 },
    { "no-text-on-term", 0, &DisplayDevice::text_on_term, 0 }, { "font", 1, 0, opt_font },
#endif

// OpenGL option
#ifdef CTH_GL
    { "mesh-size", 1, 0, opt_mesh_size }, { "texture-quality", 1, 0, opt_texture_quality },
    { "hints", 1, 0, opt_hints }, { "dither", 0, 0, opt_dither },
    { "no-dither", 0, 0, opt_no_dither }, { "shade", 0, 0, opt_shade },
    { "no-shade", 0, 0, opt_no_shade }, { "background", 1, 0, opt_background },
    { "no-background", 0, 0, opt_no_background }, { "fly", 1, 0, opt_fly },
    { "no-fly", 0, 0, opt_no_fly }, { "full-screen", 0, &fullScreen, 1 },
    { "no-full-screen", 0, &fullScreen, 0 },
#endif

    // general options
    { "path", 1, 0, 'E' }, { "ini-file", 1, 0, opt_ini_file }, { "keymap", 1, 0, opt_keymap },
    { "dbl-load", 0, &double_load.value, 1 }, { "no-dbl-load", 0, &double_load.value, 0 },
    { "save", 0, &options_save.value, 1 }, { "no-save", 0, &options_save.value, 0 },
    { "prt-file", 1, 0, opt_prt_file },
    { "joystick", 0, &Joystick::useJoystick, 1 }, { "no-joystick", 0, &Joystick::useJoystick, 0 },
    { "esc", 0, &key_esc, 1 },
    { "no-esc", 0, &key_esc, 0 }, { "verbose", 2, 0, opt_verbose },
    { "no-verbose", 1, &cthugha_verbose.value, 0 }, { "help", 0, 0, '?' },

    { 0, 0, 0, 0 }
};

int do_param(int c, int value, char* str) {
    switch (c) {
    case 0:
        return 0;

        //
        // CoreOptions
        //
    case opt_flashlight:
        flashlight.setInitialEntry(str ? str : (char*)"non-locked:on");
        break;
    case opt_no_flashlight:
    case 's':
        flashlight.setInitialEntry(str ? str : (char*)"locked:off");
        break;

    case 'f':
        CthughaBuffer::current->flame.setInitialEntry(str);
        break;

    case opt_border:
        border.setInitialEntry(str);
        break;

    case 't':
        CthughaBuffer::current->translate.setInitialEntry(str);
        break;

    case 'w':
        CthughaBuffer::current->wave.setInitialEntry(str);
        break;

    case opt_wave_scale:
        CthughaBuffer::current->waveScale.setInitialEntry(str);
        break;

    case 'o':
        CthughaBuffer::current->object.setInitialEntry(str);
        break;

    case 'p':
        CthughaBuffer::current->palette.setInitialEntry(str);
        break;

    case opt_palette_smoothing:
        paletteSmoothingChance = atof(str);
        if (paletteSmoothingChance < 0.0) {
            CTH_WARN("Palette smoothing chance below 0, clamping to 0.\n");
            paletteSmoothingChance = 0.0;
        } else if (paletteSmoothingChance > 1.0) {
            CTH_WARN("Palette smoothing chance above 1, clamping to 1.\n");
            paletteSmoothingChance = 1.0;
        }
        break;
    case opt_no_palette_smoothing:
        paletteSmoothingChance = 0.0;
        break;
    case opt_palette_set:
        if (!palette_set_filter(str))
            return 1;
        break;

    case 'd':
        screen.setInitialEntry(str);
        break;

    case 'P':
        CthughaBuffer::current->pcx.setInitialEntry(str);
        break;

    case 'a':
        CthughaBuffer::current->table.setInitialEntry(str);
        break;

    case 'm':
        audioProcessing.setInitialEntry(str);
        break;

    case opt_light:
        light.setInitialEntry(str);
        break;
    case opt_no_light:
        light.setInitialEntry("locked:none");
        break;


    case '2': /* Stereo */
        soundChannels.setValue(2);
        break;
    case '1': /* Mono */
        soundChannels.setValue(1);
        break;

    case 'v': /* Sample rate */
        soundSampleRate.setValue(value);
        break;

    case opt_sound_format:
        soundFormat.change(str);
        break;

    case opt_pulse_server:
        if (str != NULL) {
            strncpy(pulse_server, str, PATH_MAX);
            pulse_server[PATH_MAX - 1] = '\0';
        } else {
            pulse_server[0] = '\0';
        }
        break;

    case opt_pulse_latency_ms:
        pulse_latency_msec = value;
        if (pulse_latency_msec < 50) {
            CTH_WARN("PulseAudio latency below 50 ms, clamping to 50 ms.\n");
            pulse_latency_msec = 50;
        } else if (pulse_latency_msec > 10000) {
            CTH_WARN("PulseAudio latency above 10000 ms, clamping to 10000 ms.\n");
            pulse_latency_msec = 10000;
        }
        break;

    case opt_audio_output_dump:
        if (str != NULL) {
            strncpy(audio_output_dump, str, PATH_MAX);
            audio_output_dump[PATH_MAX - 1] = '\0';
        } else {
            audio_output_dump[0] = '\0';
        }
        break;

    case 'L': /* Line as input */
        mixer_initial_volume("line", value | (value << 8));
        break;
    case 'M': /* Mic as input */
        mixer_initial_volume("mic", value | (value << 8));
        break;
    case 'C': /* CD as input */
        mixer_initial_volume("cd", value | (value << 8));
        break;

    case opt_mixer: {
        char name[100];
        char* p;
        int vol;
        if ((p = strchr(str, ':')) != 0) {
            *p = ' ';
        }
        sscanf(str, "%s %d", name, &vol);
        mixer_initial_volume(name, vol);
        break;
    }

#if WITH_CDROM == 1
    case 'c': /* with cd starting at track */
        cd_first_track.change(str);
        break;
#endif

    case 'x': /* debug mode */
        soundDeviceNr.setValue(SDN_Random);
        break;


    case 'l': /* Lock changes */
        lock.setValue(1);
        break;

    case 'T':
        changeWaitMin.change(str);
        break;
    case 'R':
        changeWaitRandom.change(str);
        break;
    case 'Q':
        changeQuiet.change(str);
        break;
    case opt_msg_time:
        changeMsgTime.change(str);
        break;

    case 'r': /* sync-mode */
        display_syncwait = 1;
        break;

    case opt_fire_level:
        changeFireLevel.change(str);
        break;

    case opt_min_noise:
        sound_minnoise.change(str);
        break;

    case 'q': /* alternative quiet-strings */
        AutoChanger::loadSilenceStrings(str);
        break;

    case 'X': /* disable PCX */
        display_use_pcx = 0;
        break;

    case 'D': /* display-mode */
        if (strchr(str, 'x') == NULL) {
            /* use a predefined size */
            display_mode = value;
        } else {
            /* use a special size (only useful with X11) */
            display_mode = -1;
            disp_size.x = value;
            disp_size.y = atoi(strchr(str, 'x') + 1);
        }
        break;

    case 'S': /* buffer-size */
        if (strchr(str, 'x') == NULL) {
            /* use a predfined size */
            if (value < 0)
                value = 0;
            if (value >= nBufferSizes)
                value = nBufferSizes;
            BUFF_WIDTH = bufferSizes[value].x;
            BUFF_HEIGHT = bufferSizes[value].y;

            display_mode = max(display_mode, value);
        } else {
            /* use a special size */
            BUFF_WIDTH = value;
            BUFF_HEIGHT = atoi(strchr(str, 'x') + 1);
            if (BUFF_WIDTH < 64)
                BUFF_WIDTH = 64;
            if (BUFF_HEIGHT < 64)
                BUFF_HEIGHT = 64;
            if (BUFF_WIDTH > MAX_BUFF_WIDTH)
                BUFF_WIDTH = MAX_BUFF_WIDTH;
            if (BUFF_HEIGHT > MAX_BUFF_WIDTH)
                BUFF_HEIGHT = MAX_BUFF_WIDTH;
        }
        break;

    case opt_prt_file:
        strncpy(display_prt_file, str, PATH_MAX);
        break;

    case 'E': /* extra lib path */
        strncpy(extra_lib_path, str, PATH_MAX);
        strncat(extra_lib_path, "/", PATH_MAX);
        break;

    case opt_ini_file:
        strncpy(ini_file_override, str, PATH_MAX);
        ini_file_override[PATH_MAX - 1] = '\0';
        break;


    case opt_keymap:
        strncpy(Keymap::keymapFile, str, PATH_MAX);
        break;

    case opt_verbose:
        cthugha_verbose.change(str ? str : (char*)"4");
        break;

    case opt_test:
        use_translates.value = 0;
        display_use_pcx = 0;
        break;

    case opt_zoom:
        zoom.change(str);
        break;

    case opt_maxfps:
        maxFramesPerSecond.change(str);
        break;

    case opt_play:
        strncpy(SoundDeviceFile::name, str, PATH_MAX);
        soundDeviceNr.setValue(SDN_File);
        break;

    case opt_silent:
        soundSilent.setValue(1);
        break;
    case opt_no_silent:
        soundSilent.setValue(0);
        break;

    case opt_dev_dsp:
        strncpy(SoundDeviceDSP::dev_dsp, str, PATH_MAX);
        break;

#if WITH_DSP == 1
    case opt_dsp_sync:
        soundDSPSync.setValue(1);
        break;

    case opt_no_dsp_sync:
        soundDSPSync.setValue(0);
        break;
#endif

#if WITH_CDROM == 1
    case opt_dev_cd:
        strncpy(dev_cd, str, PATH_MAX);
        break;
#endif
#if WITH_MIXER == 1
    case opt_dev_mixer:
        strncpy(dev_mixer, str, PATH_MAX);
        break;
#endif
    case opt_fifo:
        strncpy(SoundDeviceFile::fifo, str, PATH_MAX);
        break;

    case opt_snd_fragments:
        soundDSPFragments.change(str);
        break;

#ifdef CTH_XWIN
    case opt_position:
        sscanf(str, "%d%d", &window_pos.x, &window_pos.y);
        window_do_pos = 1;
        CTH_TRACE("window pos: %d %d\n", "options", window_pos.x, window_pos.y);
        break;

    case opt_font:
        strncpy(xcth_font, str, 256);
        break;
#endif

    case opt_sound_method:
        soundDSPMethod.change(str);
        break;

    case opt_sound_buffer:
        soundBuffer.change(str);
        break;

    case opt_no_snd_buffer:
        soundBuffer.setValue(0);
        break;

#ifdef CTH_GL
    case opt_mesh_size:
        MeshSize.change(str);
        break;

    case opt_texture_quality:
        TextureQuality.change(str);
        break;

    case opt_hints:
        Hints.change(str);
        break;

    case opt_dither:
        Dither.change("on");
        break;

    case opt_no_dither:
        Dither.change("off");
        break;

    case opt_shade:
        Shade.change("smooth");
        break;

    case opt_no_shade:
        Shade.change("flat");
        break;

    case opt_background:
        background.setInitialEntry(str);
        break;

    case opt_no_background:
        background.setInitialEntry("lock:off");
        break;

    case opt_fly:
        fly.setInitialEntry(str);
        break;

    case opt_no_fly:
        fly.setInitialEntry("lock:none");
        break;
#endif

    default: /* error or help */
        return 1;
    }
    return 0;
}

/*
 * Process parameters, before anything else is done
 */
static int optindsave;
int get_pre_params(int argc, char* argv[]) {
    int c;
    int option_index = 0;

    static struct option long_options_preini[] = { { "path", 1, 0, 'E' },
        { "ini-file", 1, 0, opt_ini_file },
        { "verbose", 2, 0, opt_verbose }, { "no-verbose", 1, &cthugha_verbose.value, 0 },
#ifdef CTH_XWIN
        { "text-on-term", 0, &DisplayDevice::text_on_term, 1 },
        { "no-text-on-term", 0, &DisplayDevice::text_on_term, 0 },
#endif
        { 0, 0, 0, 0 } };

    char* argv_tmp[argc]; /* copy of argv to work with */
    optindsave = optind;
    opterr = 0; /* don't print error msgs */
    for (c = 0; c < argc; c++) /* save argv */
        argv_tmp[c] = argv[c];

    while ((c = getopt_long(argc, argv_tmp, "E:", long_options_preini, &option_index)) != -1) {
        if (do_param((c == '?') ? 0 : c, optarg ? atoi(optarg) : 0, optarg))
            return 1;
    }

    return 0;
}

/*
 *  Process programm-params
 */
int get_params(int argc, char* argv[]) {
    int c;
    int option_index = 0;

    read_ini();

    /*
     * get command line options after loading ini files
     */
    opterr = 1; /* print error msgs */
    option_index = 0;
    optind = optindsave; /* start again at first opt */

    while ((c = getopt_long(argc, argv,
                "21v:L:M:C:c:xrP:E:?S:"
                "f:w:d:p:t:o:q:T:R:D:a:m:XlQ:s"
                ,
                long_options, &option_index))
        != -1) {

        if (do_param(c, optarg ? atoi(optarg) : 0, optarg)) {
            usage();
            return 1;
        }
    }

    return 0;
}
