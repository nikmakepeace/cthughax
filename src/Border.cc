#include "cthugha.h"
#include "AudioFrame.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "VideoFilterchain.h"
#include "cth_buffer.h"

static CoreOptionEntry* border_entries[]
    = { new CoreOptionEntry("border0", ""), new CoreOptionEntry("border1", ""),
          new CoreOptionEntry("border2", ""), new CoreOptionEntry("border3", "") };
static CoreOptionEntryList borderEntries;

CoreOption border(0, "border", borderEntries, CORE_OPTION_AUTO_CHANGE);

void init_border() {
    border.add(border_entries, 4);
}

void apply_border(CthughaBuffer& buffer, const VideoFrameContext& context, int borderMode) {
    unsigned char* active = buffer.activePixels();
    if (active == 0)
        return;

    int width = buffer.width();
    int hiddenRows = buffer.hiddenBorderRows();
    int hiddenBytes = buffer.hiddenBorderByteCount();
    unsigned char* top = buffer.activeTopHiddenRows();
    unsigned char* bottom = buffer.activeBottomHiddenRows();

    switch (borderMode) {
    case 0:
        memset(bottom, 0, hiddenBytes);
        memset(top, 0, hiddenBytes);
        break;
    case 1:
        for (int i = 0; i < hiddenRows; i++) {
            memcpy(top + i * width, audioFrameRawData(), width);
            memcpy(bottom + i * width, audioFrameRawData(), width);
        }
        break;
    case 2: {
        int amplitude = (context.audioMetrics != 0) ? context.audioMetrics->amplitude : 0;
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
