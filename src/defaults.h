// Central defaults for option-facing runtime state.

#ifndef CTHUGHA_DEFAULTS_H
#define CTHUGHA_DEFAULTS_H

#include "cthugha.h"
#include "AudioTypes.h"

static const int DEFAULT_OPTION_VALUE = 0; // Unit: raw option value; used by Option::Option before concrete defaults are applied.
static const int DEFAULT_OPTION_INT_NO_MAX = 0; // Unit: OptionInt max sentinel; used by OptionInt when an integer option has no maximum.
static const int DEFAULT_OPTION_INT_MIN = 0; // Unit: raw option value; used by OptionInt when an integer option has no explicit minimum.
static const int OPTION_ON_OFF_MAX_EXCLUSIVE = 2; // Unit: exclusive raw option maximum; used by OptionOnOff for off/on values.

static const int DEFAULT_VERBOSE_LEVEL = 1; // Unit: log verbosity level; used by cthugha_verbose at startup.
static const int DEFAULT_VERBOSE_MIN_LEVEL = -1; // Unit: log verbosity level; used as the minimum accepted cthugha_verbose value.
static const char* DEFAULT_VERBOSE_COMMAND_LEVEL_TEXT = "4"; // Unit: option text; used when --verbose is provided without an explicit level.

static const int DEFAULT_OPTIONS_SAVE_ENABLED = 0; // Unit: boolean; used by the save option to decide whether to write .cthugha.auto on exit.
static const int DEFAULT_DOUBLE_LOAD_ENABLED = 0; // Unit: boolean; used by double-load to skip duplicate feature files by default.

static const int DEFAULT_CHANGE_QUIET_MS = 1500; // Unit: milliseconds; used by quiet-change before AutoChanger reacts to a quiet gap.
static const int DEFAULT_CHANGE_WAIT_MIN_MS = 3000; // Unit: milliseconds; used by min-time as AutoChanger's minimum dwell.
static const int DEFAULT_CHANGE_WAIT_RANDOM_MS = 11000; // Unit: milliseconds; used by random-time as AutoChanger's extra random dwell.
static const int DEFAULT_CHANGE_WAIT_RANDOM_MIN_MS = 1; // Unit: milliseconds; used by AutoChanger to keep random-time modulo arithmetic valid.
static const int DEFAULT_CHANGE_CUMULATIVE_FIRE_LEVEL = 1000; // Unit: cumulative fire units; used by AutoChanger's acoustic trigger threshold.
static const int DEFAULT_AUTOCHANGER_LOCKED = 0; // Unit: boolean; used by lock so automatic scene changes start enabled.
static const int DEFAULT_AUTOCHANGER_CHANGE_LITTLE = 0; // Unit: boolean; used by little so AutoChanger changes all core options by default.

static const int DEFAULT_CHANGE_MESSAGE_MS = 10000; // Unit: milliseconds; used by change-msg-time for quiet text cue spacing.
static const int DEFAULT_QUIET_MESSAGE_DURATION_MS = 6000; // Unit: milliseconds; used by VideoDirector as the maximum quiet cue display duration.
static const double DEFAULT_PALETTE_SMOOTHING_CHANCE = 1.0; // Unit: probability 0..1; used by palette-smoothing as the default transition chance.
static const int DEFAULT_PALETTE_SMOOTH_SECONDS = 2; // Unit: seconds; used by VideoDirector as the palette transition frame budget.
static const int DEFAULT_IMAGE_LOADING_ENABLED = 1; // Unit: boolean; used by VideoDirector so --images starts enabled.

static const int DEFAULT_AUDIO_INPUT_LOOP_ENABLED = 1; // Unit: boolean; used by loop/no-loop for file playback input.
static const AudioInputMode DEFAULT_AUDIO_INPUT_MODE = AIM_DSPIn; // Unit: AudioInputMode enum; used by audioInputMode and AudioSettings defaults.
static const int DEFAULT_SOUND_SAMPLE_RATE_HZ = 44000; // Unit: Hertz; used by sound-sample-rate.
static const int DEFAULT_SOUND_CHANNELS = 2; // Unit: channel count; used by sound-channels to start in stereo.
static const int SOUND_CHANNELS_MIN = 1; // Unit: channel count; used by OptionChannels as the minimum accepted channel count.
static const int SOUND_CHANNELS_MAX_EXCLUSIVE = 3; // Unit: exclusive channel count; used by OptionChannels to accept mono or stereo.
static const AudioSampleFormat DEFAULT_SOUND_FORMAT = SF_u8; // Unit: AudioSampleFormat enum; used by sound-format.
static const int DEFAULT_SOUND_DSP_METHOD = 0; // Unit: DSP method id; used by sound-method when the DSP backend is active.
static const int DEFAULT_SOUND_DSP_FRAGMENTS = 16; // Unit: fragment count; used by sound-fragments for DSP buffering.
static const int DEFAULT_SOUND_DSP_FRAGMENT_SIZE = 0; // Unit: log2 fragment size selector; used by sound-fragment-size before DSP auto-sizing.
static const int DEFAULT_SOUND_DSP_SYNC_ENABLED = 0; // Unit: boolean; used by sound-sync so DSP sync starts disabled.
static const int DEFAULT_SOUND_SILENT_ENABLED = 0; // Unit: boolean; used by silent so audio output starts audible.
static const int DEFAULT_SOUND_MINNOISE = 5; // Unit: 8-bit RMS amplitude; used by minnoise as the quiet/noisy threshold.
static const int SOUND_MINNOISE_MAX_EXCLUSIVE = 256; // Unit: 8-bit RMS amplitude; used by minnoise as the exclusive maximum.
static const int DEFAULT_MIXER_VOLUME = 0; // Unit: mixer volume value; used by OptionVolume before mixer devices report initial levels.
static const int DEFAULT_PULSE_LATENCY_MS = 2000; // Unit: milliseconds; used by pulse-latency-ms as the requested PulseAudio buffer.
static const int DEFAULT_AUDIO_NULL_TARGET_LATENCY_MS = 0; // Unit: milliseconds; used by AudioNullOutput defaultTargetLatencyMs().
static const int DEFAULT_AUDIO_PULSE_TARGET_LATENCY_MS = 250; // Unit: milliseconds; used by AudioPulseOutput defaultTargetLatencyMs().
static const int DEFAULT_AUDIO_DSP_TARGET_LATENCY_MS = 250; // Unit: milliseconds; used by AudioDSPOutput defaultTargetLatencyMs().

#define DEFAULT_PULSE_SERVER_TEXT "" // Unit: server string; used to initialize pulse_server before --pulse-server.
#define DEFAULT_AUDIO_OUTPUT_DUMP_PATH "" // Unit: filesystem path; used to initialize audio_output_dump before --audio-output-dump.
#define DEFAULT_AUDIO_INPUT_FILE_PATH "" // Unit: filesystem path; used to initialize audio_input_file before --play.

#if WITH_DSP == 1
#define DEFAULT_DSP_DEVICE_PATH DEV_DSP // Unit: filesystem path; used to initialize dev_dsp before --dev-dsp.
#else
#define DEFAULT_DSP_DEVICE_PATH "" // Unit: filesystem path; used to initialize dev_dsp when DSP support is compiled out.
#endif

#if WITH_MIXER == 1
#define DEFAULT_MIXER_DEVICE_PATH DEV_MIXER // Unit: filesystem path; used to initialize dev_mixer before --dev-mixer.
#else
#define DEFAULT_MIXER_DEVICE_PATH "" // Unit: filesystem path; used to initialize dev_mixer when mixer support is compiled out.
#endif

static const int DEFAULT_MAX_FRAMES_PER_SECOND = 25; // Unit: frames per second; used by maxFPS for frame pacing.
static const int DEFAULT_ZOOM_MODE = 0; // Unit: zoom mode; used by zoom where 0 means fit to window.
static const int ZOOM_MODE_MAX_EXCLUSIVE = 3; // Unit: zoom mode; used by zoom to accept fit, 1x, or 2x modes.
static const int DEFAULT_DISPLAY_MODE = 0; // Unit: display mode index; used by disp-mode to choose the first predefined X11 size.
static const int DEFAULT_DISPLAY_TEXT_ON_TERM = 0; // Unit: boolean; used by text-on-term so X11 text overlay is used by default.
static const int DEFAULT_ESCAPE_KEY_ENABLED = 1; // Unit: boolean; used by esc/no-esc to allow Escape to quit by default.

static const int DEFAULT_X11_OVERRIDE_REDIRECT = 0; // Unit: boolean; used by override-redirect/no-decorate so the window manager stays in charge.
static const int DEFAULT_X11_PRIVATE_CMAP = 0; // Unit: boolean; used by install/no-install so the default colormap is shared.
static const int DEFAULT_X11_MIT_SHM = 1; // Unit: boolean; used by mit-shm so shared-memory XImage transport starts enabled.
static const int DEFAULT_X11_ROOT_WINDOW = 0; // Unit: boolean; used by root/no-root so rendering starts in a normal window.
static const int DEFAULT_X11_FULLSCREEN = 0; // Unit: boolean; used by full-screen so the window starts decorated and sized.
static const int DEFAULT_X11_WINDOW_POSITION_ENABLED = 0; // Unit: boolean; used by position to mark no explicit window position by default.
static const int DEFAULT_X11_WINDOW_POSITION_X = 0; // Unit: screen pixels; used by window_pos.x before --position.
static const int DEFAULT_X11_WINDOW_POSITION_Y = 0; // Unit: screen pixels; used by window_pos.y before --position.
static const int DEFAULT_X11_PANEL_ENABLED = 0; // Unit: boolean; used by panel/no-panel so the X11 control panel starts hidden.
#define DEFAULT_X11_FONT_NAME "-adobe-courier-medium-r-normal--14-*-100-100-m-*-*-*" // Unit: XLFD font name; used by xcth_font before --font.

#if HAVE_NCURSES == 1
static const int DEFAULT_NCURSES_ENABLED = 1; // Unit: boolean; used by ncurses_use when ncurses support is compiled in.
#else
static const int DEFAULT_NCURSES_ENABLED = 0; // Unit: boolean; used by ncurses_use when ncurses support is compiled out.
#endif

#define DEFAULT_EXTRA_LIBRARY_PATH "" // Unit: filesystem path prefix; used by --path before a custom path is provided.
#define DEFAULT_INI_FILE_OVERRIDE_PATH "" // Unit: filesystem path; used by --ini-file before an override is provided.
#define DEFAULT_KEYMAP_FILE_PATH "" // Unit: filesystem path; used by Keymap before --keymap is provided.
#define DEFAULT_SCREENSHOT_FILE_PREFIX "PrintScreen" // Unit: filename prefix; used by display_prt_file before --prt-file.
#define DEFAULT_PALETTE_SET_FILTER_TEXT "" // Unit: palette set list text; used by palette-set before filters are provided.
static const int DEFAULT_PALETTE_SET_FILTER_COUNT = 0; // Unit: palette set count; used by palette-set before filters are provided.

static const int DEFAULT_USE_TRANSLATES_ENABLED = 1; // Unit: boolean; used by use-translate so translation tables are active by default.
static const int DEFAULT_USE_OBJECTS_ENABLED = 1; // Unit: boolean; used by use-objects so 3-D wave objects are active by default.
#define DEFAULT_FLASHLIGHT_ENABLE_INITIAL_ENTRY "non-locked:on" // Unit: CoreOption entry text; used by --flashlight without an explicit value.
#define DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY "locked:off" // Unit: CoreOption entry text; used by --no-flashlight without an explicit value.

static const int DEFAULT_QOTD_PREFETCH_TIMEOUT_MS = 500; // Unit: milliseconds; used by QotdMessagesProvider for non-blocking fetch attempts.
static const char* DEFAULT_QOTD_SERVER_TEXT = "djxmmx.net:17"; // Unit: host[:port] string; used by qotd-server when no server is supplied.
static const char* DEFAULT_QOTD_PORT_TEXT = "17"; // Unit: TCP port string; used by QotdMessagesProvider when the server omits a port.

#endif
