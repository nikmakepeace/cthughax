#include "cthugha.h"
#include "AudioFrame.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "VisualPipeline.h"
#include "cth_buffer.h"

static CoreOptionEntry* border_entries[]
    = { new CoreOptionEntry("border0", ""), new CoreOptionEntry("border1", ""),
          new CoreOptionEntry("border2", ""), new CoreOptionEntry("border3", "") };
static CoreOptionEntryList borderEntries;

CoreOption border(0, "border", borderEntries);

void init_border() {
    border.add(border_entries, 4);
}

void apply_border(CthughaBuffer& buffer, const VisualFrameContext& context, int borderMode) {
    unsigned char* active = buffer.activePixels();
    if (active == 0)
        return;

    int width = buffer.width();
    int height = buffer.height();
    int pitch = buffer.pitch();
    unsigned char* top = active - 3 * pitch;
    unsigned char* bottom = active + height * pitch;

    switch (borderMode) {
    case 0:
        memset(bottom, 0, 3 * pitch);
        memset(top, 0, 3 * pitch);
        break;
    case 1:
        for (int i = 0; i < 3; i++) {
            memcpy(top + i * pitch, audioFrameData(), width);
            memcpy(bottom + i * pitch, audioFrameData(), width);
        }
        break;
    case 2: {
        int amplitude = (context.audioAnalysis != 0) ? context.audioAnalysis->amplitude : 0;
        memset(bottom, amplitude, 3 * pitch);
        memset(top, amplitude, 3 * pitch);
        break;
    }
    case 3:
        memset(bottom, 255, 3 * pitch);
        memset(top, 255, 3 * pitch);
        break;
    }
}
