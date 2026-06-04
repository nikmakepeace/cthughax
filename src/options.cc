#include "cthugha.h"
#include "options.h"
#include "display.h"
#include "TranslationOptions.h"
#include "information.h"
#include "display.h"
#include "AudioOptions.h"
#include "Mixer.h"
#include "waves.h"
#include "cth_buffer.h"
#include "AutoChanger.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "AudioProcessor.h"
#include "Border.h"
#include "Flashlight.h"
#include "flames.h"
#include "DisplayDevice.h"
#include "AudioAnalyzer.h"
#include "VideoDirector.h"
#include "defaults.h"
#ifdef CTH_XWIN
#include "xcthugha.h"
#endif
#include "imath.h"

#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <vector>

enum option_nr {
    opt_verbose = 16000,
    opt_no_verbose,
    opt_play,
    opt_loop,
    opt_no_loop,
    opt_sound_device_number,
    opt_snd_fragments,
    opt_snd_fragment_size,
    opt_mixer,
    opt_msg_time,
    opt_prt_file,
    opt_cumulative_fire_level,
    opt_min_noise,
    opt_listen,
    opt_wave_scale,
    opt_position,
    opt_sound_method,
    opt_sound_format,
    opt_dev_dsp,
    opt_dev_mixer,
    opt_border,
    opt_test,
    opt_zoom,
    opt_maxfps,
    opt_flashlight,
    opt_no_flashlight,
    opt_font,
    opt_silent,
    opt_no_silent,
    opt_dsp_sync,
    opt_no_dsp_sync,
    opt_keymap,
    opt_palette_smoothing,
    opt_no_palette_smoothing,
    opt_palette_set,
    opt_pulse_server,
    opt_pulse_latency_ms,
    opt_audio_output_dump,
    opt_ini_file,
    opt_esc,
    opt_no_esc,
    opt_image,
    opt_images,
    opt_no_images,
    opt_random_noise,
    opt_qotd,
    opt_no_qotd,
    opt_qotd_server,
#ifdef CTH_XWIN
    opt_x11_mit_shm,
    opt_x11_no_mit_shm,
    opt_x11_root,
    opt_x11_no_root,
    opt_x11_install,
    opt_x11_no_install,
    opt_x11_override_redirect,
    opt_x11_no_override_redirect,
    opt_x11_full_screen,
    opt_x11_no_full_screen,
    opt_x11_panel,
    opt_x11_no_panel,
    opt_x11_text_on_term,
    opt_x11_no_text_on_term,
#endif
};

struct option long_options[] = {
    // selecting sound device options
    { "no-sound", 0, 0, 'x' },
    { "random-noise", 0, 0, opt_random_noise },
    { "sound-device-number", 1, 0, opt_sound_device_number },

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
    { "sound-fragment-size", 1, 0, opt_snd_fragment_size },
    { "snd-fragment-size", 1, 0, opt_snd_fragment_size },
    { "dev-dsp", 1, 0, opt_dev_dsp },
#endif

    // Play/exec options
    { "loop", 0, 0, opt_loop }, { "no-loop", 0, 0, opt_no_loop },

// Mixer options
#if WITH_MIXER == 1
    { "dev-mixer", 1, 0, opt_dev_mixer }, { "mixer", 1, 0, opt_mixer }, { "line", 1, 0, 'L' },
    { "mic", 1, 0, 'M' },
#endif

// auto changer options
    { "lock", 0, &lock.value, 1 }, { "no-lock", 0, &lock.value, 0 },
    { "little", 0, &change_little.value, 1 }, { "no-little", 0, &change_little.value, 0 },
    { "min-time", 1, 0, 'T' }, { "random-time", 1, 0, 'R' },
    { "msg-time", 1, 0, opt_msg_time }, { "quiet-time", 1, 0, 'Q' },
    { "quiet-file", 1, 0, 'q' }, { "min-noise", 1, 0, opt_min_noise },
    { "qotd", 0, 0, opt_qotd }, { "no-qotd", 0, 0, opt_no_qotd },
    { "qotd-server", 1, 0, opt_qotd_server },
    { "cumulative-fire-level", 1, 0, opt_cumulative_fire_level },

// Effect Controls
    { "flashlight", 0, 0, opt_flashlight }, { "no-flashlight", 0, 0, opt_no_flashlight },
    { "sound-processing", 1, 0, 'm' }, { "flame", 1, 0, 'f' }, { "translation", 1, 0, 't' },

// buffer options
    { "buff-size", 1, 0, 'S' }, { "wave", 1, 0, 'w' },
    { "wave-scale", 1, 0, opt_wave_scale }, { "object", 1, 0, 'o' },
    { "no-object", 0, &use_objects.value, 0 },
    { "use-object", 0, &use_objects.value, 1 }, { "trans", 0, &use_translates.value, 1 },
    { "no-trans", 0, &use_translates.value, 0 },
    { "table", 1, 0, 'a' }, { "border", 1, 0, opt_border },

// display options
    { "disp-mode", 1, 0, 'D' }, { "palette", 1, 0, 'p' }, { "display", 1, 0, 'd' },
    { "image", 1, 0, opt_image }, { "images", 0, 0, opt_images },
    { "no-images", 0, 0, opt_no_images },
    { "palette-smoothing", 1, 0, opt_palette_smoothing },
    { "no-palette-smoothing", 0, 0, opt_no_palette_smoothing },
    { "palette-set", 1, 0, opt_palette_set },
    { "test", 0, 0, opt_test }, { "max-fps", 1, 0, opt_maxfps },
    { "show-fps", 0, &showFPS.value, 1 },
    { "no-show-fps", 0, &showFPS.value, 0 },
    { "zoom", 1, 0, opt_zoom },

// X11 options
#ifdef CTH_XWIN
    { "mit-shm", 0, 0, opt_x11_mit_shm },
    { "no-mit-shm", 0, 0, opt_x11_no_mit_shm },
    { "root", 0, 0, opt_x11_root },
    { "no-root", 0, 0, opt_x11_no_root },
    { "install", 0, 0, opt_x11_install },
    { "no-install", 0, 0, opt_x11_no_install },
    { "override-redirect", 0, 0, opt_x11_override_redirect },
    { "no-override-redirect", 0, 0, opt_x11_no_override_redirect },
    { "no-decorate", 0, 0, opt_x11_override_redirect },
    { "decorate", 0, 0, opt_x11_no_override_redirect },
    { "full-screen", 0, 0, opt_x11_full_screen },
    { "no-full-screen", 0, 0, opt_x11_no_full_screen },
    { "panel", 0, 0, opt_x11_panel },
    { "no-panel", 0, 0, opt_x11_no_panel }, { "position", 1, 0, opt_position },
    { "text-on-term", 0, 0, opt_x11_text_on_term },
    { "no-text-on-term", 0, 0, opt_x11_no_text_on_term },
    { "font", 1, 0, opt_font },
#endif

    // general options
    { "path", 1, 0, 'E' }, { "ini-file", 1, 0, opt_ini_file }, { "keymap", 1, 0, opt_keymap },
    { "dbl-load", 0, &double_load.value, 1 }, { "no-dbl-load", 0, &double_load.value, 0 },
    { "save", 0, &options_save.value, 1 }, { "no-save", 0, &options_save.value, 0 },
    { "prt-file", 1, 0, opt_prt_file },
    { "esc", 0, 0, opt_esc },
    { "no-esc", 0, 0, opt_no_esc }, { "verbose", 2, 0, opt_verbose },
    { "no-verbose", 0, 0, opt_no_verbose }, { "help", 0, 0, '?' },

    { 0, 0, 0, 0 }
};

int do_param(int c, int value, char* str) {
    switch (c) {
    case 0:
        return 0;

        //
        // EffectControls
        //
    case opt_flashlight:
        break;
    case opt_no_flashlight:
    case 's':
        break;

    case 'f':
        break;

    case opt_border:
        break;

    case 't':
        break;

    case 'w':
        break;

    case opt_wave_scale:
        break;

    case 'o':
        break;

    case 'p':
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
        break;

    case opt_image:
        break;

    case 'a':
        break;

    case 'm':
        audioProcessing.setInitialEntry(str);
        break;

    case '2': /* Stereo */
        break;
    case '1': /* Mono */
        break;

    case 'v': /* Sample rate */
        break;

    case opt_sound_format:
        break;

    case opt_pulse_server:
        break;

    case opt_pulse_latency_ms:
        break;

    case opt_audio_output_dump:
        break;

    case 'L': /* Line as input */
        break;
    case 'M': /* Mic as input */
        break;
    case opt_mixer:
        break;

    case 'x': /* no sound input */
        break;

    case opt_random_noise:
    case opt_sound_device_number:
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

    case opt_cumulative_fire_level:
        changeCumulativeFireLevel.change(str);
        break;

    case opt_min_noise:
        break;

    case 'q': /* alternative quiet-strings */
        videoDirector().silenceMessages().loadFile(str);
        break;

    case opt_qotd:
        videoDirector().silenceMessages().setQotdEnabled(1);
        break;

    case opt_no_qotd:
        videoDirector().silenceMessages().setQotdEnabled(0);
        break;

    case opt_qotd_server:
        videoDirector().silenceMessages().setQotdServer(str);
        break;

    case opt_images:
        videoDirector().setImageLoadingEnabled(1);
        break;

    case opt_no_images:
        videoDirector().setImageLoadingEnabled(0);
        break;

    case 'D': /* display-mode */
        break;

    case 'S': /* buffer-size */
        break;

    case opt_prt_file:
        strncpy(display_prt_file, str, PATH_MAX);
        break;

    case 'E': /* extra lib path */
        break;

    case opt_ini_file:
        break;


    case opt_keymap:
        break;

    case opt_esc:
    case opt_no_esc:
        break;

    case opt_verbose:
    case opt_no_verbose:
        break;

    case opt_test:
        use_translates.value = 0;
        videoDirector().setImageLoadingEnabled(0);
        break;

    case opt_zoom:
        zoom.change(str);
        break;

    case opt_maxfps:
        maxFramesPerSecond.change(str);
        break;

    case opt_play:
    case opt_loop:
    case opt_no_loop:
        break;

    case opt_silent:
    case opt_no_silent:
        break;

    case opt_dev_dsp:
        break;

#if WITH_DSP == 1
    case opt_dsp_sync:
    case opt_no_dsp_sync:
        break;
#endif

#if WITH_MIXER == 1
    case opt_dev_mixer:
        break;
#endif
    case opt_snd_fragments:
    case opt_snd_fragment_size:
        break;

#ifdef CTH_XWIN
    case opt_x11_mit_shm:
    case opt_x11_no_mit_shm:
    case opt_x11_root:
    case opt_x11_no_root:
    case opt_x11_install:
    case opt_x11_no_install:
    case opt_x11_override_redirect:
    case opt_x11_no_override_redirect:
    case opt_x11_full_screen:
    case opt_x11_no_full_screen:
    case opt_x11_panel:
    case opt_x11_no_panel:
    case opt_x11_text_on_term:
    case opt_x11_no_text_on_term:
        break;

    case opt_position:
        break;

    case opt_font:
        break;
#endif

    case opt_sound_method:
        break;

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
        { "verbose", 2, 0, opt_verbose }, { "no-verbose", 0, 0, opt_no_verbose },
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

int params_request_help(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0)
            return 1;
    }

    return 0;
}

#ifdef CTH_XWIN
static int x_toolkit_option_with_arg(const char* arg) {
    static const char* options[] = {
        "-background",
        "-bg",
        "-display",
        "-fn",
        "-font",
        "-foreground",
        "-fg",
        "-geometry",
        "-name",
        "-selectionTimeout",
        "-title",
        "-xrm",
        "-xnllanguage",
        "-xtsessionID",
        0
    };

    for (int i = 0; options[i] != 0; i++) {
        if (strcmp(arg, options[i]) == 0)
            return 1;
    }

    return 0;
}

static int x_toolkit_option_without_arg(const char* arg) {
    static const char* options[] = {
        "+rv",
        "+synchronous",
        "-iconic",
        "-reverse",
        "-rv",
        "-synchronous",
        0
    };

    for (int i = 0; options[i] != 0; i++) {
        if (strcmp(arg, options[i]) == 0)
            return 1;
    }

    return 0;
}

static void filter_x_toolkit_options(int argc, char* argv[], std::vector<char*>& filtered) {
    filtered.clear();
    if (argc <= 0)
        return;

    filtered.push_back(argv[0]);
    for (int i = 1; i < argc; i++) {
        if (x_toolkit_option_with_arg(argv[i])) {
            if (i + 1 < argc)
                i++;
            continue;
        }

        if (x_toolkit_option_without_arg(argv[i]))
            continue;

        filtered.push_back(argv[i]);
    }
}
#endif

/*
 *  Process programm-params
 */
int get_params(int argc, char* argv[], const Config& config) {
    int c;
    int option_index = 0;
#ifdef CTH_XWIN
    std::vector<char*> filteredArgv;
    filter_x_toolkit_options(argc, argv, filteredArgv);
    int parseArgc = int(filteredArgv.size());
    char** parseArgv = filteredArgv.empty() ? argv : filteredArgv.data();
#else
    int parseArgc = argc;
    char** parseArgv = argv;
#endif

    read_ini(config.paths);

    /*
     * get command line options after loading ini files
     */
    opterr = 1; /* print error msgs */
    option_index = 0;
    optind = optindsave ? optindsave : 1; /* start again at first opt */

    while ((c = getopt_long(parseArgc, parseArgv,
                "21v:L:M:xE:?S:"
                "f:w:d:p:t:o:q:T:R:D:a:m:lQ:s"
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
