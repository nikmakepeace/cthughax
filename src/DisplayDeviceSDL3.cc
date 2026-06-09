/** @file
 * SDL3 display backend for Linux-first windowed presentation.
 */

#include "DisplayDeviceSDL3.h"

#include "BitmapFont.h"
#include "Configuration.h"
#include "CthughaDisplay.h"
#include "DisplayBackend.h"
#include "DisplayDevice.h"
#include "DisplayOverlay.h"
#include "DisplayPresentation.h"
#include "DisplayPresentationOptions.h"
#include "DisplayRuntime.h"
#include "DisplaySystem.h"
#include "FramePalette.h"
#include "IndexedDisplayFrame.h"
#include "InputQueue.h"
#include "InterfaceRuntime.h"
#include "ProcessServices.h"
#include "Sdl3Presentation.h"
#include "ViewportPresentation.h"
#include "cthugha.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>

#ifndef CTH_XWIN
int cth_init(int*, char**) {
    return 0;
}
#endif

static PixelSize configuredDisplaySize(const DisplayConfig& config) {
    if (config.hasCustomDisplaySize)
        return PixelSize(config.displayWidth, config.displayHeight);

    int mode = config.displayMode;
    if (mode < 0 || mode >= nScreenSizes)
        mode = 0;

    return PixelSize::fromXy(screenSizes[mode]);
}

static const char* writeRawKeyCharacter(char value, char* buffer,
    size_t bufferSize) {
    if (buffer == 0 || bufferSize < 2)
        return "";

    buffer[0] = value;
    buffer[1] = '\0';
    return buffer;
}

static char shiftedAsciiForKey(SDL_Keycode key) {
    switch (key) {
    case SDLK_APOSTROPHE:
        return '"';
    case SDLK_COMMA:
        return '<';
    case SDLK_MINUS:
        return '_';
    case SDLK_PERIOD:
        return '>';
    case SDLK_SLASH:
        return '?';
    case SDLK_SEMICOLON:
        return ':';
    case SDLK_EQUALS:
        return '+';
    case SDLK_LEFTBRACKET:
        return '{';
    case SDLK_BACKSLASH:
        return '|';
    case SDLK_RIGHTBRACKET:
        return '}';
    case SDLK_GRAVE:
        return '~';
    default:
        return 0;
    }
}

static const char* rawKeyTextForSdlKey(SDL_Keycode key, int shifted,
    char* buffer, size_t bufferSize) {
    switch (key) {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_RETURN2:
        return "Return";
    case SDLK_ESCAPE:
        return writeRawKeyCharacter(char(27), buffer, bufferSize);
    case SDLK_BACKSPACE:
    case SDLK_KP_BACKSPACE:
        return "BackSpace";
    case SDLK_DELETE:
        return "Delete";
    case SDLK_UP:
        return "Up";
    case SDLK_DOWN:
        return "Down";
    case SDLK_LEFT:
        return "Left";
    case SDLK_RIGHT:
        return "Right";
    case SDLK_PAGEUP:
    case SDLK_PRIOR:
        return "Prior";
    case SDLK_PAGEDOWN:
        return "Next";
    case SDLK_HOME:
        return "Home";
    case SDLK_END:
        return "End";
    case SDLK_PRINTSCREEN:
        return "Print";
    case SDLK_KP_0:
        return "KP_0";
    case SDLK_KP_1:
        return "KP_1";
    case SDLK_KP_2:
        return "KP_2";
    case SDLK_KP_3:
        return "KP_3";
    case SDLK_KP_4:
        return "KP_4";
    case SDLK_KP_5:
        return "KP_5";
    case SDLK_KP_6:
        return "KP_6";
    case SDLK_KP_7:
        return "KP_7";
    case SDLK_KP_8:
        return "KP_8";
    case SDLK_KP_9:
        return "KP_9";
    case SDLK_KP_DIVIDE:
        return "/";
    case SDLK_KP_MULTIPLY:
        return "*";
    case SDLK_KP_MINUS:
        return "-";
    case SDLK_KP_PLUS:
        return "+";
    case SDLK_F1:
        return "F1";
    case SDLK_F2:
        return "F2";
    case SDLK_F3:
        return "F3";
    case SDLK_F4:
        return "F4";
    case SDLK_F5:
        return "F5";
    case SDLK_F6:
        return "F6";
    case SDLK_F7:
        return "F7";
    case SDLK_F8:
        return "F8";
    case SDLK_F9:
        return "F9";
    case SDLK_F10:
        return "F10";
    case SDLK_F11:
        return "F11";
    case SDLK_F12:
        return "F12";
    case SDLK_F13:
        return "F13";
    case SDLK_F14:
        return "F14";
    case SDLK_F15:
        return "F15";
    case SDLK_F16:
        return "F16";
    case SDLK_F17:
        return "F17";
    case SDLK_F18:
        return "F18";
    case SDLK_F19:
        return "F19";
    case SDLK_F20:
        return "F20";
    case SDLK_F21:
        return "F21";
    case SDLK_F22:
        return "F22";
    case SDLK_F23:
        return "F23";
    case SDLK_F24:
        return "F24";
    default:
        break;
    }

    if (key >= SDLK_A && key <= SDLK_Z) {
        char value = char((shifted ? 'A' : 'a') + (key - SDLK_A));
        return writeRawKeyCharacter(value, buffer, bufferSize);
    }

    if (shifted) {
        char shiftedAscii = shiftedAsciiForKey(key);
        if (shiftedAscii != 0)
            return writeRawKeyCharacter(shiftedAscii, buffer, bufferSize);
    }

    if (key > 0 && key < 127)
        return writeRawKeyCharacter(char(key), buffer, bufferSize);

    return "";
}

class Sdl3FrameDump {
    std::string directory;
    int directoryReady;
    int frame;
    int dumped;
    int limit;
    int every;
    LogSink& log;

    int ensureDirectory() {
        if (directoryReady)
            return 1;

        directoryReady = 1;
        if (mkdir(directory.c_str(), 0777) != 0 && errno != EEXIST) {
            log.errorErrno(errno,
                "Can not create SDL3 frame dump directory `%s'.\n",
                directory.c_str());
            directory.clear();
            return 0;
        }

        return 1;
    }

public:
    Sdl3FrameDump(const SDL3Config& config, LogSink& log_)
        : directory(config.frameDumpDirectory)
        , directoryReady(0)
        , frame(0)
        , dumped(0)
        , limit(std::max(config.frameDumpLimit, 1))
        , every(std::max(config.frameDumpEvery, 1))
        , log(log_) {
    }

    void dump(const PixelSize& size, const unsigned char* rgba, int pitch) {
        if (directory.empty() || rgba == 0 || !size.valid())
            return;
        if (!ensureDirectory())
            return;

        frame++;
        if ((frame % every) != 0 || dumped >= limit)
            return;

        char path[1024];
        std::snprintf(path, sizeof(path), "%s/frame-%06d.ppm",
            directory.c_str(), frame);
        FILE* file = std::fopen(path, "wb");
        if (file == 0) {
            log.errorErrno(errno, "Can not create SDL3 frame dump `%s'.\n",
                path);
            directory.clear();
            return;
        }

        std::fprintf(file, "P6\n%d %d\n255\n", size.width, size.height);
        for (int y = 0; y < size.height; ++y) {
            const unsigned char* row = rgba + y * pitch;
            for (int x = 0; x < size.width; ++x)
                std::fwrite(row + x * 4, 1, 3, file);
        }
        std::fclose(file);
        dumped++;
    }
};

class DisplayDeviceSDL3 : public DisplayDevice {
public:
    explicit DisplayDeviceSDL3(const DisplayConfig& config) {
        PixelSize requestedSize = configuredDisplaySize(config);
        disp_size = requestedSize.toXy();
        bypp = 4;
        bytes_per_line = requestedSize.width * bypp;
        draw_mode = DM_mapped4;

        const BitmapFont& font = dosVga9x14Font();
        fontSize.x = font.glyphWidth;
        fontSize.y = font.glyphHeight;
        text_size.x = 0;
        text_size.y = 0;
    }
};

class DisplayBackendSDL3 : public DisplayBackend {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    PixelSize outputSizeValue;
    PixelSize textureSize;
    Sdl3RgbaColor rgbaPalette[256];
    std::vector<unsigned char> rgbaPixels;
    SDL3Config config;
    Sdl3FrameDump frameDump;
    LogSink& log;
    int initialized;
    int videoInitialized;

    void destroyTexture() {
        if (texture != 0)
            SDL_DestroyTexture(texture);
        texture = 0;
        textureSize = PixelSize();
    }

    void refreshOutputSize() {
        if (window == 0)
            return;

        int width = outputSizeValue.width;
        int height = outputSizeValue.height;
        if (SDL_GetWindowSizeInPixels(window, &width, &height)
            && width > 0 && height > 0) {
            outputSizeValue = PixelSize(width, height);
            disp_size = outputSizeValue.toXy();
            bytes_per_line = outputSizeValue.width * bypp;
            text_size.x = 0;
            text_size.y = 0;
        }
    }

    int ensureTexture(const PixelSize& size) {
        if (texture != 0 && textureSize == size)
            return 1;

        destroyTexture();
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STREAMING, size.width, size.height);
        if (texture == 0) {
            log.error("Can not create SDL3 frame texture: %s\n",
                SDL_GetError());
            return 0;
        }

        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        textureSize = size;
        return 1;
    }

    void updatePalette(FramePalette* framePalette) {
        if (framePalette == 0)
            return;

        if (framePalette->paletteDirty())
            sdl3PopulateRgbaPalette(framePalette->currentPalette(),
                rgbaPalette, 256);
        framePalette->clearPaletteDirty();
    }

    void uploadFrame(const IndexedDisplayFrame& frame) {
        PixelSize frameSize(frame.width(), frame.height());
        int pitch = frameSize.width * 4;
        rgbaPixels.resize(pitch * frameSize.height);
        sdl3ExpandIndexedPixelsToRgba(frame.pixels(), frameSize,
            frame.pitch(), rgbaPalette, &rgbaPixels[0], pitch);
        frameDump.dump(frameSize, &rgbaPixels[0], pitch);

        if (!SDL_UpdateTexture(texture, 0, &rgbaPixels[0], pitch))
            log.warn("SDL3 frame texture update failed: %s\n", SDL_GetError());
    }

    void setOverlayColor(int color) {
        switch (color) {
        case TEXT_COLOR_ERROR:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        case TEXT_COLOR_HIGHLIGHT:
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            break;
        case TEXT_COLOR_NORMAL:
        default:
            SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
            break;
        }
    }

    void renderGlyph(const BitmapFont& font, unsigned char character,
        int x, int y) {
        for (int row = 0; row < font.glyphHeight; ++row) {
            uint16_t mask = font.row(character, row);
            for (int column = 0; column < font.glyphWidth; ++column) {
                if ((mask & (uint16_t(1)
                        << (font.glyphWidth - 1 - column))) == 0)
                    continue;

                SDL_FRect rect;
                rect.x = float(x + column);
                rect.y = float(y + row);
                rect.w = 1.0f;
                rect.h = 1.0f;
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    void renderOverlayLine(const BitmapFont& font, const char* text,
        int length, int x, int y) {
        for (int i = 0; i < length; ++i)
            renderGlyph(font, (unsigned char)text[i],
                x + i * font.glyphWidth, y);
    }

    void renderOverlayCommand(const OverlayTextCommand& command,
        const OverlayLayout& layout) {
        setOverlayColor(command.color);
        const BitmapFont& font = dosVga9x14Font();
        const char* lineStart = command.text.c_str();
        const char* lineEnd;
        double yLine = command.y;

        do {
            lineEnd = std::strchr(lineStart, '\n');
            int length = lineEnd ? int(lineEnd - lineStart)
                                 : int(std::strlen(lineStart));
            if (length == 0)
                return;

            int x = sdl3OverlayTextX(length, command.justification, layout);
            int y = sdl3OverlayTextY(yLine, layout);
            renderOverlayLine(font, lineStart, length, x, y);

            yLine += 1.0;
            lineStart = lineEnd + 1;
        } while (lineEnd != 0);
    }

    void renderOverlayCommands(const OverlayCommands& overlays) {
        const BitmapFont& font = dosVga9x14Font();
        OverlayLayout layout(outputSizeValue.width / font.glyphWidth,
            outputSizeValue.height / font.glyphHeight, font.glyphWidth,
            font.glyphHeight);
        for (size_t i = 0; i < overlays.count(); ++i)
            renderOverlayCommand(overlays.at(i), layout);
    }

public:
    DisplayBackendSDL3(const DisplayConfig& displayConfig,
        const SDL3Config& sdl3Config, LogSink& log_)
        : window(0)
        , renderer(0)
        , texture(0)
        , outputSizeValue(configuredDisplaySize(displayConfig))
        , textureSize()
        , config(sdl3Config)
        , frameDump(sdl3Config, log_)
        , log(log_)
        , initialized(0)
        , videoInitialized(0) {
        sdl3PopulateRgbaPalette(ColorPalette(), rgbaPalette, 256);

        SDL_SetAppMetadata("Cthugha", PACKAGE_VERSION,
            "org.cthughanix.cthugha");
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            log.error("Can not initialize SDL3 video: %s\n", SDL_GetError());
            return;
        }
        videoInitialized = 1;

        SDL_WindowFlags flags = 0;
        if (config.resizableWindowEnabled)
            flags |= SDL_WINDOW_RESIZABLE;
        if (config.highPixelDensityEnabled)
            flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

        window = SDL_CreateWindow("Cthugha", outputSizeValue.width,
            outputSizeValue.height, flags);
        if (window == 0) {
            log.error("Can not create SDL3 window: %s\n", SDL_GetError());
            return;
        }

        const char* rendererName = config.rendererName.empty()
            ? 0
            : config.rendererName.c_str();
        renderer = SDL_CreateRenderer(window, rendererName);
        if (renderer == 0) {
            log.error("Can not create SDL3 renderer: %s\n", SDL_GetError());
            return;
        }

        refreshOutputSize();
        initialized = 1;
    }

    virtual ~DisplayBackendSDL3() {
        destroyTexture();
        if (renderer != 0)
            SDL_DestroyRenderer(renderer);
        if (window != 0)
            SDL_DestroyWindow(window);
        if (videoInitialized)
            SDL_Quit();
    }

    int isInitialized() const {
        return initialized;
    }

    virtual DisplayEventStats processEvents(InputEventSink& input) {
        DisplayEventStats stats;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            stats.eventCount++;
            switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                stats.closeRequested = 1;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                stats.resizeEvents++;
                refreshOutputSize();
                break;
            case SDL_EVENT_WINDOW_EXPOSED:
                stats.exposeEvents++;
                break;
            case SDL_EVENT_KEY_DOWN: {
                char text[8] = "";
                int shifted = (event.key.mod & SDL_KMOD_SHIFT) != 0;
                const char* rawText = rawKeyTextForSdlKey(event.key.key,
                    shifted, text, sizeof(text));
                input.pushRawKey(rawText, shifted);
                break;
            }
            default:
                break;
            }
        }
        return stats;
    }

    virtual PixelSize outputSize() const {
        return outputSizeValue;
    }

    virtual void present(const DisplayPresentation& presentation) {
        refreshOutputSize();

        if (!presentation.frame.valid())
            return;

        PixelSize frameSize(presentation.frame.width(),
            presentation.frame.height());
        if (!ensureTexture(frameSize))
            return;

        updatePalette(presentation.framePalette);
        uploadFrame(presentation.frame);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        PixelRect destination = ViewportPresentation::drawCopyRect(
            presentation.viewport);
        if (!destination.valid())
            destination = PixelRect(0, 0, outputSizeValue.width,
                outputSizeValue.height);

        SDL_FRect destinationRect;
        destinationRect.x = float(destination.x);
        destinationRect.y = float(destination.y);
        destinationRect.w = float(destination.width);
        destinationRect.h = float(destination.height);
        SDL_RenderTexture(renderer, texture, 0, &destinationRect);

        renderOverlayCommands(presentation.overlays);
        SDL_RenderPresent(renderer);
    }
};

class CthughaDisplaySDL3 : public CthughaDisplay {
    SecondsClock& clock;
    InterfaceRuntime& interfaceRuntime;
    ErrorMessages& errorMessages;

public:
    CthughaDisplaySDL3(DisplayDevice& device, DisplayRuntime& runtime,
        SecondsClock& clock_, DisplayPresentationSettings& settings_,
        InterfaceRuntime& interfaceRuntime_, ErrorMessages& errorMessages_)
        : CthughaDisplay(device, runtime, clock_, settings_)
        , clock(clock_)
        , interfaceRuntime(interfaceRuntime_)
        , errorMessages(errorMessages_) {
    }

    virtual void operator()() {
        DisplayDevice& target = device();
        DisplayRuntime& stage = runtime();
        int traceDisplayTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
        double displayTiming[6] = { 0, 0, 0, 0, 0, 0 };
        if (traceDisplayTiming)
            displayTiming[0] = clock.nowSeconds();

        target.setGlobalPalette();
        if (traceDisplayTiming)
            displayTiming[1] = clock.nowSeconds();

        composePresentationFrame();
        if (traceDisplayTiming)
            displayTiming[2] = clock.nowSeconds();

        buffer = buffer0;
        bufferWidth = indexedDisplayFrameValue.pitch();
        checkZoom();
        if (traceDisplayTiming)
            displayTiming[3] = clock.nowSeconds();

        const BitmapFont& font = dosVga9x14Font();
        PixelSize output = stage.outputSize();
        fontSize.x = font.glyphWidth;
        fontSize.y = font.glyphHeight;
        text_size.x = output.width / font.glyphWidth;
        text_size.y = output.height / font.glyphHeight;
        OverlayLayout overlayLayout(text_size.x, text_size.y, fontSize.x,
            fontSize.y);
        DisplayStatusSnapshot overlayStatus(status(),
            currentFrameDeltaSeconds());
        OverlayCommands overlays = collectDisplayOverlayCommands(fps,
            interfaceRuntime, errorMessages, overlayLayout, overlayStatus,
            settings());
        if (traceDisplayTiming)
            displayTiming[4] = clock.nowSeconds();

        stage.present(indexedDisplayFrameValue, displayViewport(),
            target.needsFullCopy, needsClear, overlays);
        needsClear = 0;
        if (traceDisplayTiming) {
            displayTiming[5] = clock.nowSeconds();
            const DisplayViewport& viewport = displayViewport();
            CTH_TRACE("sdl3 frame-ms=%.3f palette-ms=%.3f compose-ms=%.3f viewport-ms=%.3f overlay-ms=%.3f present-ms=%.3f size=%dx%d draw=%dx%d\n",
                "display timing",
                (displayTiming[5] - displayTiming[0]) * 1000.0,
                (displayTiming[1] - displayTiming[0]) * 1000.0,
                (displayTiming[2] - displayTiming[1]) * 1000.0,
                (displayTiming[3] - displayTiming[2]) * 1000.0,
                (displayTiming[4] - displayTiming[3]) * 1000.0,
                (displayTiming[5] - displayTiming[4]) * 1000.0,
                viewport.windowSize.width, viewport.windowSize.height,
                viewport.drawSize.width, viewport.drawSize.height);
        }
    }
};

class SDL3DisplayDriverFactory : public DisplayDriverFactory {
    SDL3Config config;

public:
    explicit SDL3DisplayDriverFactory(const SDL3Config& config_)
        : config(config_) {
    }

    virtual DisplayDriverId driverId() const {
        return DisplayDriverSDL3;
    }

    virtual const char* driverName() const {
        return "sdl3";
    }

    virtual std::unique_ptr<DisplaySystemComponents> open(
        const DisplayOpenRequest& request) {
        std::unique_ptr<DisplayDeviceSDL3> device(
            new DisplayDeviceSDL3(request.config));
        std::unique_ptr<DisplayBackendSDL3> backend(
            new DisplayBackendSDL3(request.config, config, request.log));
        if (!backend->isInitialized())
            return std::unique_ptr<DisplaySystemComponents>();

        DisplayDeviceSDL3& deviceRef = *device;
        DisplayBackendSDL3& backendRef = *backend;
        std::unique_ptr<DisplayRuntime> runtime(
            new DisplayRuntime(backendRef));
        std::unique_ptr<CthughaDisplay> coordinator(
            new CthughaDisplaySDL3(deviceRef, *runtime, request.clock,
                request.presentationSettings, request.interfaceRuntime,
                request.errorMessages));

        return std::unique_ptr<DisplaySystemComponents>(
            new DisplaySystemComponents(std::move(device),
                std::move(backend), std::move(runtime),
                std::move(coordinator)));
    }
};

std::unique_ptr<DisplayDriverFactory> newSDL3DisplayDriverFactory(
    const SDL3Config& config) {
    return std::unique_ptr<DisplayDriverFactory>(
        new SDL3DisplayDriverFactory(config));
}
