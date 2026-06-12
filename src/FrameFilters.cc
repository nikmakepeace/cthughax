#include "BorderRenderer.h"
#include "Flame.h"
#include "FlashlightRenderer.h"
#include "FramePalette.h"
#include "BitmapFont.h"
#include "IndexedImage.h"
#include "ProcessServices.h"
#include "FrameFilters.h"
#include "Wave.h"

#include <string.h>
#include <vector>

ImageFilter::ImageFilter()
    : image(0)
    , placement() { }

const char* ImageFilter::name() const {
    return "image";
}

FrameFilterBufferContract ImageFilter::bufferContract() const {
    return FrameFilterPixelCopiedSource;
}

void ImageFilter::setImage(const IndexedImage* image_) {
    image = image_;
}

void ImageFilter::setPlacement(const ImagePlacement& placement_) {
    placement = placement_;
}

void ImageFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "executing image stage\n");
    if (image == 0 || !placement.visible())
        return;

    FrameStageBuffer& buffer = frame.buffer();
    unsigned char* destinationPixels = buffer.destinationPixels();
    const unsigned char* sourcePixels = image->pixels();
    if (destinationPixels == 0 || sourcePixels == 0)
        return;

    for (int row = 0; row < placement.height; row++) {
        const unsigned char* source = sourcePixels
            + (placement.sourceY + row) * image->width()
            + placement.sourceX;
        unsigned char* destination = destinationPixels
            + (placement.destinationY + row) * buffer.pitch()
            + placement.destinationX;

        memcpy(destination, source, placement.width);
    }
}

FlameFilter::FlameFilter()
    : flame(0)
    , generalFlame(0) { }

const char* FlameFilter::name() const {
    return "flame";
}

FrameFilterBufferContract FlameFilter::bufferContract() const {
    return FrameFilterPixelCopiedSource;
}

void FlameFilter::setFlame(const Flame* flame_) {
    flame = flame_;
}

void FlameFilter::setGeneralFlame(int generalFlame_) {
    generalFlame = generalFlame_;
}

void FlameFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "executing flame stage\n");

    if (flame != 0)
        flame->execute(frame.buffer(), frame.context(), generalFlame, lookupTables);
}

TranslateFilter::TranslateFilter()
    : translate() { }

const char* TranslateFilter::name() const {
    return "translate";
}

FrameFilterBufferContract TranslateFilter::bufferContract() const {
    return translate.ready()
        ? FrameFilterPixelFreshDestination
        : FrameFilterPixelCopiedSource;
}

void TranslateFilter::setTranslate(const TranslationTable& table) {
    translate = Translate(table);
}

void TranslateFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "executing translate stage\n");
    translate.execute(frame.buffer(), frame.context());
}

WaveFilter::WaveFilter()
    : wave(0)
    , config()
    , state()
    , lookupTables()
    , randomSourceValue(0)
    , configured(0)
    , needsConfiguration(1) { }

const char* WaveFilter::name() const {
    return "wave";
}

FrameFilterBufferContract WaveFilter::bufferContract() const {
    return FrameFilterPixelCopiedSource;
}

void WaveFilter::setWave(Wave* wave_, const WaveConfig& config_) {
    if (wave != wave_) {
        state.clear();
        needsConfiguration = 1;
    } else if (!configured || !config.sameAs(config_)) {
        needsConfiguration = 1;
    }

    wave = wave_;
    config = config_;
    configured = 1;
}

void WaveFilter::setRandomSource(RandomSource& randomSource) {
    randomSourceValue = &randomSource;
}

void WaveFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "executing wave stage\n");
    if (wave != NULL && randomSourceValue != 0) {
        wave->execute(frame.buffer(), frame.context(), config,
            needsConfiguration, state, lookupTables, *randomSourceValue,
            frame.log());
        needsConfiguration = 0;
    }
}

TextInjectionFilter::TextInjectionFilter()
    : font(&dosVga9x14Font())
    , message()
    , framesRemaining(0)
    , inkColor(-1)
    , marginPixels(0)
    , horizontalAlign(TextInjectionAlignCenter)
    , verticalAlign(TextInjectionAlignMiddle) { }

const char* TextInjectionFilter::name() const {
    return "text-injection";
}

FrameFilterBufferContract TextInjectionFilter::bufferContract() const {
    return FrameFilterPixelCopiedSource;
}

void TextInjectionFilter::setMessage(const char* message_, int frameCount) {
    if (message_ == 0 || frameCount <= 0) {
        message.clear();
        framesRemaining = 0;
        return;
    }

    message = message_;
    framesRemaining = frameCount;
}

void TextInjectionFilter::setInkColor(int color) {
    if (color < 0)
        inkColor = -1;
    else if (color > 255)
        inkColor = 255;
    else
        inkColor = color;
}

void TextInjectionFilter::setPlacement(TextInjectionHorizontalAlign horizontalAlign_,
    TextInjectionVerticalAlign verticalAlign_, int marginPixels_) {
    horizontalAlign = horizontalAlign_;
    verticalAlign = verticalAlign_;
    marginPixels = (marginPixels_ < 0) ? 0 : marginPixels_;
}

static int textInjectionWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

static int textInjectionAddLine(std::vector<std::string>& lines,
    const std::string& line, int maxLines) {
    if (int(lines.size()) >= maxLines)
        return 0;

    lines.push_back(line);
    return 1;
}

static int textInjectionWrapWords(const std::string& message, int maxColumns,
    int maxLines, std::vector<std::string>& lines) {
    std::string line;
    int sawText = 0;
    unsigned int i = 0;

    while (i < message.size()) {
        char c = message[i];

        if (c == '\r') {
            i++;
            continue;
        }

        if (c == '\n') {
            if (!textInjectionAddLine(lines, line, maxLines))
                return 0;
            line.clear();
            i++;
            continue;
        }

        if (textInjectionWhitespace(c)) {
            i++;
            continue;
        }

        unsigned int wordStart = i;
        while (i < message.size() && message[i] != '\n' && message[i] != '\r'
            && !textInjectionWhitespace(message[i]))
            i++;

        std::string word = message.substr(wordStart, i - wordStart);
        sawText = 1;

        if (int(word.size()) > maxColumns)
            return 0;

        if (line.empty()) {
            line = word;
        } else if (int(line.size() + 1 + word.size()) <= maxColumns) {
            line += ' ';
            line += word;
        } else {
            if (!textInjectionAddLine(lines, line, maxLines))
                return 0;
            line = word;
        }
    }

    if (!line.empty()) {
        if (!textInjectionAddLine(lines, line, maxLines))
            return 0;
    }

    return sawText && !lines.empty();
}

static int textInjectionHorizontalPosition(int rectX, int rectWidth, int lineWidth,
    TextInjectionHorizontalAlign align) {
    switch (align) {
    case TextInjectionAlignLeft:
        return rectX;
    case TextInjectionAlignRight:
        return rectX + rectWidth - lineWidth;
    case TextInjectionAlignCenter:
    default:
        return rectX + (rectWidth - lineWidth) / 2;
    }
}

static int textInjectionVerticalPosition(int rectY, int rectHeight, int blockHeight,
    TextInjectionVerticalAlign align) {
    int spareHeight = rectHeight - blockHeight;
    if (spareHeight < 0)
        spareHeight = 0;

    switch (align) {
    case TextInjectionAlignTop:
        return rectY;
    case TextInjectionAlignBottom:
        return rectY + spareHeight;
    case TextInjectionAlignMiddle:
    default:
        if (blockHeight > (rectHeight * 2) / 3)
            return rectY + spareHeight / 4;
        if (blockHeight > rectHeight / 3)
            return rectY + spareHeight / 3;
        return rectY + spareHeight / 2;
    }
}

static int textInjectionInkColor(const FramePalette* framePalette, int requestedColor) {
    if (requestedColor >= 0)
        return requestedColor;

    if (framePalette == 0)
        return 255;

    const ColorPalette& palette = framePalette->currentPalette();
    int bestColor = 255;
    int bestBrightness = -1;

    for (int i = 1; i < 256; i++) {
        int brightness = int(palette.component(i, 0)) + int(palette.component(i, 1))
            + int(palette.component(i, 2));
        if (brightness > bestBrightness) {
            bestBrightness = brightness;
            bestColor = i;
        }
    }

    return bestBrightness > 0 ? bestColor : 255;
}

static void textInjectionDrawLine(FrameStageBuffer& buffer, const BitmapFont& font,
    const std::string& line, int x, int y, int color) {
    unsigned char* pixels = buffer.destinationPixels();
    if (pixels == 0)
        return;

    for (unsigned int characterIndex = 0; characterIndex < line.size(); characterIndex++) {
        unsigned char character = (unsigned char)line[characterIndex];
        int glyphX = x + int(characterIndex) * font.glyphWidth;

        for (int row = 0; row < font.glyphHeight; row++) {
            uint16_t mask = font.row(character, row);
            unsigned char* destination = pixels + (y + row) * buffer.pitch() + glyphX;

            for (int column = 0; column < font.glyphWidth; column++) {
                if (mask & (uint16_t(1) << (font.glyphWidth - 1 - column)))
                    destination[column] = (unsigned char)color;
            }
        }
    }
}

void TextInjectionFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain",
        "executing text stage frames-remaining=%d\n",
        framesRemaining);

    if (framesRemaining <= 0 || message.empty())
        return;

    FrameStageBuffer& buffer = frame.buffer();
    if (font == 0 || font->glyphWidth <= 0 || font->glyphHeight <= 0
        || buffer.destinationPixels() == 0) {
        framesRemaining = 0;
        return;
    }

    int rectX = marginPixels;
    int rectY = marginPixels;
    int rectWidth = buffer.width() - 2 * marginPixels;
    int rectHeight = buffer.height() - 2 * marginPixels;
    int maxColumns = rectWidth / font->glyphWidth;
    int maxLines = rectHeight / font->glyphHeight;

    std::vector<std::string> lines;
    if (rectWidth <= 0 || rectHeight <= 0 || maxColumns <= 0 || maxLines <= 0
        || !textInjectionWrapWords(message, maxColumns, maxLines, lines)) {
        frame.log().warn("text injection rejected: message does not fit %dx%d buffer.\n",
            buffer.width(), buffer.height());
        framesRemaining = 0;
        return;
    }

    int blockHeight = int(lines.size()) * font->glyphHeight;
    int y = textInjectionVerticalPosition(rectY, rectHeight, blockHeight, verticalAlign);
    int color = textInjectionInkColor(frame.framePalette(), inkColor);

    for (unsigned int i = 0; i < lines.size(); i++) {
        int lineWidth = int(lines[i].size()) * font->glyphWidth;
        int x = textInjectionHorizontalPosition(rectX, rectWidth, lineWidth, horizontalAlign);
        textInjectionDrawLine(buffer, *font, lines[i], x, y + int(i) * font->glyphHeight,
            color);
    }

    framesRemaining--;
}

FrameCommitFilter::FrameCommitFilter()
    : flameName("unknown")
    , waveName("unknown")
    , waveScaleName("unknown")
    , tableName("unknown")
    , debugReports(0) { }

const char* FrameCommitFilter::name() const {
    return "frame-commit";
}

FrameFilterBufferContract FrameCommitFilter::bufferContract() const {
    return FrameFilterCommitBoundary;
}

void FrameCommitFilter::setSceneNames(const char* flameName_, const char* waveName_,
    const char* waveScaleName_, const char* tableName_) {
    flameName = (flameName_ != 0) ? flameName_ : "unknown";
    waveName = (waveName_ != 0) ? waveName_ : "unknown";
    waveScaleName = (waveScaleName_ != 0) ? waveScaleName_ : "unknown";
    tableName = (tableName_ != 0) ? tableName_ : "unknown";
}

void FrameCommitFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "committing indexed buffer frame\n");
    FrameStageBuffer& buffer = frame.buffer();

    if (frame.log().debugEnabled() && (debugReports < 16)) {
        int nonzero = 0;
        int peak = 0;
        for (int y = 0; y < buffer.height(); y++) {
            const unsigned char* row = buffer.destinationRow(y);
            for (int x = 0; x < buffer.width(); x++) {
                int value = row[x];
                if (value != 0)
                    nonzero++;
                if (value > peak)
                    peak = value;
            }
        }
        debugReports++;
        frame.log().debug("visual buffer: wave=%s wave-scale=%s flame=%s table=%s nonzero-pixels=%d peak-pixel=%d size=%d\n",
            waveName,
            waveScaleName,
            flameName,
            tableName,
            nonzero, peak, buffer.size());
    }
}

FlashlightFilter::FlashlightFilter() { }

const char* FlashlightFilter::name() const {
    return "flashlight";
}

FrameFilterBufferContract FlashlightFilter::bufferContract() const {
    return FrameFilterPaletteOnly;
}

void FlashlightFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "executing flashlight stage\n");
    FramePalette* framePalette = frame.framePalette();
    if (framePalette != 0)
        apply_flashlight(*framePalette, frame.context());
}

BorderFilter::BorderFilter()
    : borderMode(0) { }

const char* BorderFilter::name() const {
    return "border";
}

FrameFilterBufferContract BorderFilter::bufferContract() const {
    return FrameFilterPixelCopiedSource;
}

void BorderFilter::setBorderMode(int borderMode_) {
    borderMode = borderMode_;
}

void BorderFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain",
        "executing border stage mode=%d\n", borderMode);
    apply_border(frame.buffer(), frame.context(), borderMode);
}

PaletteFilter::PaletteFilter() { }

const char* PaletteFilter::name() const {
    return "palette";
}

FrameFilterBufferContract PaletteFilter::bufferContract() const {
    return FrameFilterPaletteOnly;
}

FramePalette& PaletteFilter::framePalette() {
    return framePaletteValue;
}

int PaletteFilter::needsTarget(const ColorPalette& palette) const {
    return !transition.hasTarget(palette);
}

void PaletteFilter::setTargetPalette(const ColorPalette& palette, int frameBudget,
    const PaletteTransitionStrategy& strategy) {
    transition.achieve(palette, frameBudget, strategy);
}

void PaletteFilter::snapThenTransitionPalette(const ColorPalette& immediatePalette,
    const ColorPalette& targetPalette, int frameBudget,
    const PaletteTransitionStrategy& strategy) {
    transition.snapThenAchieve(immediatePalette, targetPalette, frameBudget,
        strategy);
}

void PaletteFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "executing palette stage\n");
    FramePalette* framePalette = frame.framePalette();
    transition.execute((framePalette != 0) ? *framePalette : framePaletteValue);
}

IndexedFrameFilter::IndexedFrameFilter() { }

const char* IndexedFrameFilter::name() const {
    return "indexed-frame";
}

FrameFilterBufferContract IndexedFrameFilter::bufferContract() const {
    return FrameFilterPublish;
}

void IndexedFrameFilter::execute(FrameFilterFrame& frame) {
    frame.log().trace("frame filterchain", "publishing indexed frame\n");
    FrameStageBuffer& buffer = frame.buffer();
    frame.publishIndexedFrame(IndexedFrame(buffer.sourcePixels(),
        buffer.width(), buffer.height(), buffer.pitch(), frame.framePalette()));
}

FramePalette* framePaletteFromFilterchain(FrameFilterchain& filterchain) {
    return filterchain.framePalette();
}
