#include "cthugha.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "CthughaFrameBuffer.h"
#include "CthughaDisplay.h"
#include "Flashlight.h"
#include "VisualDirector.h"
#include "cth_buffer.h"
#include "imath.h"
#include "waves.h"

double paletteSmoothingChance = 1.0;
static int imageStageRequested = 0;

static void bindSelectedBuffer(CthughaFrameBuffer& frameBuffer) {
    CthughaBuffer::current = CthughaBuffer::buffers + CthughaBuffer::nCurrent;
    CthughaBuffer::current->bindFrameBuffer(frameBuffer);
}

static void bindBuffer(CthughaFrameBuffer& frameBuffer, int bufferIndex) {
    CthughaBuffer::current = CthughaBuffer::buffers + bufferIndex;
    CthughaBuffer::current->bindFrameBuffer(frameBuffer);
}

class ImageStageModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        (void)context;

        CTH_TRACE("executing image stage\n", "visual pipeline");
        show_pcx(frameBuffer);
    }
};

class BufferFrameBeginModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        (void)context;

        CTH_TRACE("beginning indexed buffer frame\n", "visual pipeline");
        for (int j = 0; j < CthughaBuffer::nBuffers; j++)
            CthughaBuffer::buffers[j].done_translate = 0;

        bindSelectedBuffer(frameBuffer);
    }
};

class FlameStageModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        CTH_TRACE("executing flame stage\n", "visual pipeline");

        for (int j = 0; j < CthughaBuffer::nBuffers; j++) {
            bindBuffer(frameBuffer, j);

            FlameEntry* flame = static_cast<FlameEntry*>(CthughaBuffer::current->flame.current());
            if (flame != 0)
                flame->execute(frameBuffer, context);
        }

        bindSelectedBuffer(frameBuffer);
    }
};

class TranslateStageModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        CTH_TRACE("executing translate stage\n", "visual pipeline");
        for (int j = 0; j < CthughaBuffer::nBuffers; j++) {
            bindBuffer(frameBuffer, j);
            TranslateEntry* translate = NULL;
            if (CthughaBuffer::current->translate.prepareCurrentEntry(translate) == 0
                && translate != NULL)
                translate->execute(frameBuffer, context);
            CthughaBuffer::current->bindFrameBuffer(frameBuffer);
        }

        bindSelectedBuffer(frameBuffer);
    }
};

class WaveStageModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        CTH_TRACE("executing wave stage\n", "visual pipeline");
        for (int j = 0; j < CthughaBuffer::nBuffers; j++) {
            bindBuffer(frameBuffer, j);
            WaveEntry* wave = static_cast<WaveEntry*>(CthughaBuffer::current->wave.current());
            if (wave != NULL)
                wave->execute(frameBuffer, context);
        }

        bindSelectedBuffer(frameBuffer);
    }
};

class BufferFrameEndModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        (void)context;

        CTH_TRACE("ending indexed buffer frame\n", "visual pipeline");
        static int debugReports = 0;

        for (int j = 0; j < CthughaBuffer::nBuffers; j++) {
            CthughaBuffer::current = CthughaBuffer::buffers + j;

            if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
                int nonzero = 0;
                int peak = 0;
                for (int i = 0; i < BUFF_SIZE; i++) {
                    int value = CthughaBuffer::current->activeBuffer[i];
                    if (value != 0)
                        nonzero++;
                    if (value > peak)
                        peak = value;
                }
                debugReports++;
                CTH_DEBUG("visual buffer: buffer=%d wave=%s wave-scale=%s flame=%s table=%s nonzero-pixels=%d peak-pixel=%d size=%d\n",
                    j, CthughaBuffer::current->wave.currentName(),
                    CthughaBuffer::current->waveScale.currentName(),
                    CthughaBuffer::current->flame.currentName(),
                    CthughaBuffer::current->table.currentName(),
                    nonzero, peak, BUFF_SIZE);
            }

            unsigned char* t = CthughaBuffer::current->activeBuffer;
            CthughaBuffer::current->activeBuffer = CthughaBuffer::current->passiveBuffer;
            CthughaBuffer::current->passiveBuffer = t;
        }

        bindSelectedBuffer(frameBuffer);
    }
};

class FlashlightVisualModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        CTH_TRACE("executing flashlight stage\n", "visual pipeline");
        apply_flashlight(frameBuffer, context);
    }
};

class BorderVisualModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        CTH_TRACE("executing border stage mode=%d\n", "visual pipeline", int(border));
        apply_border(frameBuffer, context);
    }
};

class PaletteStageModule : public VisualModule {
public:
    void execute(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
        (void)context;

        CoreOption* paletteOption = frameBuffer.paletteOption();
        Palette* currentPalette = frameBuffer.palette();
        int* lastPalette = frameBuffer.lastPalette();

        if (paletteOption == 0 || currentPalette == 0 || lastPalette == 0)
            return;

        int selectedPalette = paletteOption->currentN();
        if ((*lastPalette == selectedPalette) && (frameBuffer.paletteChanged() == 0))
            return;

        Palette* desiredPalette = &(((PaletteEntry*)paletteOption->current())->pal);

        if (*lastPalette != selectedPalette) {
            *lastPalette = selectedPalette;
            if (((double)rand() / ((double)RAND_MAX + 1.0)) >= paletteSmoothingChance) {
                // Skip smoothing, jump directly to the new palette (DOS behaviour).
                frameBuffer.setPalette(*desiredPalette);
                return;
            }
        }

        const int PALETTE_CHANNEL_RANGE = 256;
        const int PALETTE_SMOOTH_SECONDS = 2;

        static int oldMaxChange = 1;
        int maxChange = (cthughaDisplay->fps > 0)
            ? max(int((double)(PALETTE_CHANNEL_RANGE / PALETTE_SMOOTH_SECONDS) / cthughaDisplay->fps), 1)
            : oldMaxChange;
        oldMaxChange = maxChange;

        frameBuffer.setPaletteChanged(256 * 3);
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 3; j++) {
                int d = (*desiredPalette)[i][j] - (*currentPalette)[i][j];
                if (d == 0)
                    frameBuffer.setPaletteChanged(frameBuffer.paletteChanged() - 1);
                else {
                    if (d < -maxChange)
                        d = -maxChange;
                    else if (d > maxChange)
                        d = maxChange;
                    (*currentPalette)[i][j] += d;
                }
            }
        }
    }
};

VisualPlan::VisualPlan() { }

void VisualPlan::append(Stage stage) {
    sequenceValue.push_back(stage);
}

int VisualPlan::includes(Stage stage) const {
    for (unsigned int i = 0; i < sequenceValue.size(); i++) {
        if (sequenceValue[i] == stage)
            return 1;
    }

    return 0;
}

VisualPlan VisualDirector::planDefaultPipeline() const {
    VisualPlan plan;

    plan.append(VisualPlan::BufferFrameBeginStage);
    plan.append(VisualPlan::ImageStage);
    plan.append(VisualPlan::FlashlightStage);
    plan.append(VisualPlan::BorderStage);
    plan.append(VisualPlan::FlameStage);
    plan.append(VisualPlan::TranslateStage);
    plan.append(VisualPlan::WaveStage);
    plan.append(VisualPlan::BufferFrameEndStage);
    plan.append(VisualPlan::PaletteStage);

    CTH_TRACE("planned default stages=%d\n", "visual director", int(plan.sequence().size()));
    return plan;
}

void VisualDirector::requestImageStage() {
    imageStageRequested = 1;
}

int VisualDirector::consumeImageStageRequest() {
    int requested = imageStageRequested;
    imageStageRequested = 0;
    return requested;
}

void VisualDirector::configurePipeline(VisualPipeline& pipeline) const {
    pipeline.setStageMode(VisualPlan::BufferFrameBeginStage, VisualStageEnabled);
    if (consumeImageStageRequest())
        pipeline.setStageMode(VisualPlan::ImageStage, VisualStageArmedOnce);
    pipeline.setStageMode(VisualPlan::FlashlightStage,
        (int(flashlight) != 0) ? VisualStageEnabled : VisualStageDisabled);
    pipeline.setStageMode(VisualPlan::BorderStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPlan::FlameStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPlan::TranslateStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPlan::WaveStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPlan::BufferFrameEndStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPlan::PaletteStage, VisualStageEnabled);
}

VisualPipelineFactory::VisualPipelineFactory() { }

VisualPipeline* VisualPipelineFactory::create(const VisualPlan& plan) const {
    VisualPipeline* pipeline = new VisualPipeline();

    pipeline->setStageSequence(plan.sequence());

    if (plan.includes(VisualPlan::BufferFrameBeginStage))
        pipeline->add(VisualPlan::BufferFrameBeginStage, new BufferFrameBeginModule(), 1);
    if (plan.includes(VisualPlan::ImageStage))
        pipeline->add(VisualPlan::ImageStage, new ImageStageModule(), 1);
    if (plan.includes(VisualPlan::FlashlightStage))
        pipeline->add(VisualPlan::FlashlightStage, new FlashlightVisualModule(), 1);
    if (plan.includes(VisualPlan::BorderStage))
        pipeline->add(VisualPlan::BorderStage, new BorderVisualModule(), 1);
    if (plan.includes(VisualPlan::FlameStage))
        pipeline->add(VisualPlan::FlameStage, new FlameStageModule(), 1);
    if (plan.includes(VisualPlan::TranslateStage))
        pipeline->add(VisualPlan::TranslateStage, new TranslateStageModule(), 1);
    if (plan.includes(VisualPlan::WaveStage))
        pipeline->add(VisualPlan::WaveStage, new WaveStageModule(), 1);
    if (plan.includes(VisualPlan::BufferFrameEndStage))
        pipeline->add(VisualPlan::BufferFrameEndStage, new BufferFrameEndModule(), 1);
    if (plan.includes(VisualPlan::PaletteStage))
        pipeline->add(VisualPlan::PaletteStage, new PaletteStageModule(), 1);

    CTH_TRACE("created pipeline=%p stages=%d modules=%d\n", "visual pipeline factory",
        pipeline, int(plan.sequence().size()), pipeline->size());
    return pipeline;
}

void VisualPipelineFactory::refresh(VisualPipeline& pipeline, const VisualPlan& plan) const {
    CTH_TRACE("refreshing pipeline=%p stages=%d modules=%d\n", "visual pipeline factory",
        &pipeline, int(plan.sequence().size()), pipeline.size());
    pipeline.refresh();
}
