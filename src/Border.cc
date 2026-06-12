#include "cthugha.h"
#include "Border.h"
#include "FrameStageBuffer.h"
#include "FrameGeneratorContext.h"
#include "cth_buffer.h"

static EffectChoice* border_entries[]
    = { new EffectChoice("border0", ""), new EffectChoice("border1", ""),
          new EffectChoice("border2", ""), new EffectChoice("border3", "") };
static EffectChoiceList borderEntries;

EffectControl border(0, "border", borderEntries, EFFECT_CONTROL_AUTO_CHANGE);

void init_border() {
    border.add(border_entries, 4);
}

static int audioBorderBytesAvailable(const FrameGeneratorContext& context) {
    if (context.audioFrame() != 0)
        return context.audioFrame()->samples * int(sizeof(char2));

    return context.rawAudioData() != 0 ? 1024 * int(sizeof(char2)) : 0;
}

static void copyAudioBorderRow(unsigned char* destination, int width, int pitch,
    const FrameGeneratorContext& context) {
    const unsigned char* rawBytes
        = reinterpret_cast<const unsigned char*>(context.rawAudioData());
    int available = audioBorderBytesAvailable(context);
    int copyBytes = available < width ? available : width;

    if (rawBytes != 0 && copyBytes > 0)
        memcpy(destination, rawBytes, copyBytes);
    if (copyBytes < width)
        memset(destination + copyBytes, 0, width - copyBytes);
    if (width < pitch)
        memset(destination + width, 0, pitch - width);
}

void apply_border(FrameStageBuffer& buffer, const FrameGeneratorContext& context, int borderMode) {
    unsigned char* destination = buffer.destinationPixels();
    if (destination == 0)
        return;

    int width = buffer.width();
    int pitch = buffer.pitch();
    int hiddenRows = buffer.hiddenBorderRows();
    int hiddenBytes = buffer.hiddenBorderByteCount();
    unsigned char* top = buffer.destinationTopHiddenRows();
    unsigned char* bottom = buffer.destinationBottomHiddenRows();

    switch (borderMode) {
    case 0:
        memset(bottom, 0, hiddenBytes);
        memset(top, 0, hiddenBytes);
        break;
    case 1:
        for (int i = 0; i < hiddenRows; i++) {
            copyAudioBorderRow(top + i * pitch, width, pitch, context);
            copyAudioBorderRow(bottom + i * pitch, width, pitch, context);
        }
        break;
    case 2: {
        int amplitude = context.audioAnalysis().amplitude();
        memset(bottom, amplitude, hiddenBytes);
        memset(top, amplitude, hiddenBytes);
        break;
    }
    case 3:
        memset(bottom, 255, hiddenBytes);
        memset(top, 255, hiddenBytes);
        break;
    }
}
