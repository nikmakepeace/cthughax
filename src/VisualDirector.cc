#include "cthugha.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "Flashlight.h"
#include "VisualDirector.h"
#include "cth_buffer.h"
#include "imath.h"
#include "waves.h"

double paletteSmoothingChance = 1.0;

static CthughaBuffer* selectedBuffer() {
    int bufferIndex = int(CthughaBuffer::nCurrent);
    if (bufferIndex < 0 || bufferIndex >= CthughaBuffer::maxNBuffers)
        return 0;

    return CthughaBuffer::buffers + bufferIndex;
}

static int activeBufferCount() {
    if (CthughaBuffer::nBuffers < 0)
        return 0;
    return min(CthughaBuffer::nBuffers, CthughaBuffer::maxNBuffers);
}

struct FlameStageBinding {
    CthughaBuffer* buffer;
    FlameEntry* flame;

    FlameStageBinding(CthughaBuffer* buffer_, FlameEntry* flame_)
        : buffer(buffer_)
        , flame(flame_) { }
};

struct TranslateStageBinding {
    CthughaBuffer* buffer;
    TranslateOption* translate;

    TranslateStageBinding(CthughaBuffer* buffer_, TranslateOption* translate_)
        : buffer(buffer_)
        , translate(translate_) { }
};

struct WaveStageBinding {
    CthughaBuffer* buffer;
    WaveEntry* wave;

    WaveStageBinding(CthughaBuffer* buffer_, WaveEntry* wave_)
        : buffer(buffer_)
        , wave(wave_) { }
};

template <class Module>
static Module* stageModule(VisualPipeline& pipeline, VisualPlan::Stage stage) {
    return dynamic_cast<Module*>(pipeline.stageModule(stage));
}

class ImageStageModule : public VisualModule {
    CthughaBuffer* buffer;
    PCXEntry* image;

public:
    ImageStageModule()
        : buffer(0)
        , image(0) { }

    void setImage(CthughaBuffer* buffer_, PCXEntry* image_) {
        buffer = buffer_;
        image = image_;
    }

    void execute(const VisualFrameContext& context) {
        (void)context;

        CTH_TRACE("executing image stage\n", "visual pipeline");
        if (buffer != 0 && image != 0)
            image->overlay(*buffer);
    }
};

class BufferFrameBeginModule : public VisualModule {
    CthughaBuffer* buffer;

public:
    BufferFrameBeginModule()
        : buffer(0) { }

    void setSelectedBuffer(CthughaBuffer* buffer_) {
        buffer = buffer_;
    }

    void execute(const VisualFrameContext& context) {
        (void)context;

        CTH_TRACE("beginning indexed buffer frame\n", "visual pipeline");
        if (buffer != 0)
            CthughaBuffer::current = buffer;
    }
};

class FlameStageModule : public VisualModule {
    std::vector<FlameStageBinding> flames;

public:
    FlameStageModule() { }

    void setFlames(const std::vector<FlameStageBinding>& flames_) {
        flames = flames_;
    }

    void execute(const VisualFrameContext& context) {
        CTH_TRACE("executing flame stage\n", "visual pipeline");

        for (unsigned int j = 0; j < flames.size(); j++) {
            if (flames[j].buffer != 0 && flames[j].flame != 0)
                flames[j].flame->execute(*flames[j].buffer, context);
        }
    }
};

class TranslateStageModule : public VisualModule {
    std::vector<TranslateStageBinding> translates;

public:
    TranslateStageModule() { }

    void setTranslateProviders(const std::vector<TranslateStageBinding>& translates_) {
        translates = translates_;
    }

    void execute(const VisualFrameContext& context) {
        CTH_TRACE("executing translate stage\n", "visual pipeline");
        for (unsigned int j = 0; j < translates.size(); j++) {
            TranslateEntry* translate = NULL;
            if (translates[j].buffer != 0
                && translates[j].translate != 0
                && translates[j].translate->prepareCurrentEntry(translate) == 0
                && translate != NULL)
                translate->execute(*translates[j].buffer, context);
        }
    }
};

class WaveStageModule : public VisualModule {
    std::vector<WaveStageBinding> waves;

public:
    WaveStageModule() { }

    void setWaves(const std::vector<WaveStageBinding>& waves_) {
        waves = waves_;
    }

    void execute(const VisualFrameContext& context) {
        CTH_TRACE("executing wave stage\n", "visual pipeline");
        for (unsigned int j = 0; j < waves.size(); j++) {
            if (waves[j].buffer != 0 && waves[j].wave != NULL)
                waves[j].wave->execute(*waves[j].buffer, context);
        }
    }
};

class BufferFrameEndModule : public VisualModule {
    std::vector<CthughaBuffer*> buffers;

public:
    BufferFrameEndModule() { }

    void setBuffers(const std::vector<CthughaBuffer*>& buffers_) {
        buffers = buffers_;
    }

    void execute(const VisualFrameContext& context) {
        (void)context;

        CTH_TRACE("ending indexed buffer frame\n", "visual pipeline");
        static int debugReports = 0;

        for (unsigned int j = 0; j < buffers.size(); j++) {
            CthughaBuffer* buffer = buffers[j];
            if (buffer == 0)
                continue;

            if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
                int nonzero = 0;
                int peak = 0;
                for (int i = 0; i < BUFF_SIZE; i++) {
                    int value = buffer->activePixels()[i];
                    if (value != 0)
                        nonzero++;
                    if (value > peak)
                        peak = value;
                }
                debugReports++;
                CTH_DEBUG("visual buffer: buffer=%d wave=%s wave-scale=%s flame=%s table=%s nonzero-pixels=%d peak-pixel=%d size=%d\n",
                    j, buffer->wave.currentName(),
                    buffer->waveScale.currentName(),
                    buffer->flame.currentName(),
                    buffer->table.currentName(),
                    nonzero, peak, BUFF_SIZE);
            }

            buffer->swapBuffers();
        }
    }
};

class FlashlightVisualModule : public VisualModule {
    CthughaBuffer* buffer;

public:
    FlashlightVisualModule()
        : buffer(0) { }

    void setBuffer(CthughaBuffer* buffer_) {
        buffer = buffer_;
    }

    void execute(const VisualFrameContext& context) {
        CTH_TRACE("executing flashlight stage\n", "visual pipeline");
        if (buffer != 0)
            apply_flashlight(*buffer, context);
    }
};

class BorderVisualModule : public VisualModule {
    CthughaBuffer* buffer;
    CoreOption* borderOption;

public:
    BorderVisualModule()
        : buffer(0)
        , borderOption(0) { }

    void setBorder(CthughaBuffer* buffer_, CoreOption* borderOption_) {
        buffer = buffer_;
        borderOption = borderOption_;
    }

    void execute(const VisualFrameContext& context) {
        int borderMode = (borderOption != 0) ? int(*borderOption) : 0;
        CTH_TRACE("executing border stage mode=%d\n", "visual pipeline", borderMode);
        if (buffer != 0)
            apply_border(*buffer, context, borderMode);
    }
};

class PaletteStageModule : public VisualModule {
    CoreOption* paletteOption;
    Palette* currentPalette;
    int* paletteChanged;
    int* lastPalette;

public:
    PaletteStageModule()
        : paletteOption(0)
        , currentPalette(0)
        , paletteChanged(0)
        , lastPalette(0) { }

    void setPalette(CoreOption* paletteOption_, Palette* currentPalette_,
        int* paletteChanged_, int* lastPalette_) {
        paletteOption = paletteOption_;
        currentPalette = currentPalette_;
        paletteChanged = paletteChanged_;
        lastPalette = lastPalette_;
    }

    void execute(const VisualFrameContext& context) {
        (void)context;

        if (paletteOption == 0 || currentPalette == 0 || paletteChanged == 0 || lastPalette == 0)
            return;

        int selectedPalette = paletteOption->currentN();
        if ((*lastPalette == selectedPalette) && (*paletteChanged == 0))
            return;

        Palette* desiredPalette = &(((PaletteEntry*)paletteOption->current())->pal);

        if (*lastPalette != selectedPalette) {
            *lastPalette = selectedPalette;
            if (((double)rand() / ((double)RAND_MAX + 1.0)) >= paletteSmoothingChance) {
                // Skip smoothing, jump directly to the new palette (DOS behaviour).
                memcpy(*currentPalette, *desiredPalette, sizeof(Palette));
                *paletteChanged = 1;
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

        *paletteChanged = 256 * 3;
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 3; j++) {
                int d = (*desiredPalette)[i][j] - (*currentPalette)[i][j];
                if (d == 0)
                    (*paletteChanged)--;
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

int VisualDirector::pcxSelectionChanged() {
    int bufferIndex = int(CthughaBuffer::nCurrent);
    if (bufferIndex < 0 || bufferIndex >= CthughaBuffer::maxNBuffers)
        return 0;

    if (int(pcxSelectionByBuffer.size()) < CthughaBuffer::maxNBuffers)
        pcxSelectionByBuffer.resize(CthughaBuffer::maxNBuffers, -1);

    int selectedPcx = CthughaBuffer::buffers[bufferIndex].pcx.currentN();
    if (pcxSelectionByBuffer[bufferIndex] == selectedPcx)
        return 0;

    pcxSelectionByBuffer[bufferIndex] = selectedPcx;
    return 1;
}

void VisualDirector::bindPipelineStages(VisualPipeline& pipeline) {
    CthughaBuffer* selected = selectedBuffer();
    std::vector<CthughaBuffer*> buffers;
    std::vector<FlameStageBinding> flames;
    std::vector<TranslateStageBinding> translates;
    std::vector<WaveStageBinding> waves;

    for (int j = 0; j < activeBufferCount(); j++) {
        CthughaBuffer* buffer = CthughaBuffer::buffers + j;

        buffers.push_back(buffer);
        flames.push_back(FlameStageBinding(buffer,
            static_cast<FlameEntry*>(buffer->flame.current())));
        translates.push_back(TranslateStageBinding(buffer, &buffer->translate));
        waves.push_back(WaveStageBinding(buffer,
            static_cast<WaveEntry*>(buffer->wave.current())));
    }

    BufferFrameBeginModule* beginModule
        = stageModule<BufferFrameBeginModule>(pipeline, VisualPlan::BufferFrameBeginStage);
    if (beginModule != 0)
        beginModule->setSelectedBuffer(selected);

    ImageStageModule* imageModule
        = stageModule<ImageStageModule>(pipeline, VisualPlan::ImageStage);
    if (imageModule != 0)
        imageModule->setImage(selected,
            (selected != 0) ? static_cast<PCXEntry*>(selected->pcx.current()) : 0);

    FlameStageModule* flameModule
        = stageModule<FlameStageModule>(pipeline, VisualPlan::FlameStage);
    if (flameModule != 0)
        flameModule->setFlames(flames);

    TranslateStageModule* translateModule
        = stageModule<TranslateStageModule>(pipeline, VisualPlan::TranslateStage);
    if (translateModule != 0)
        translateModule->setTranslateProviders(translates);

    WaveStageModule* waveModule
        = stageModule<WaveStageModule>(pipeline, VisualPlan::WaveStage);
    if (waveModule != 0)
        waveModule->setWaves(waves);

    BufferFrameEndModule* endModule
        = stageModule<BufferFrameEndModule>(pipeline, VisualPlan::BufferFrameEndStage);
    if (endModule != 0)
        endModule->setBuffers(buffers);

    FlashlightVisualModule* flashlightModule
        = stageModule<FlashlightVisualModule>(pipeline, VisualPlan::FlashlightStage);
    if (flashlightModule != 0)
        flashlightModule->setBuffer(selected);

    BorderVisualModule* borderModule
        = stageModule<BorderVisualModule>(pipeline, VisualPlan::BorderStage);
    if (borderModule != 0)
        borderModule->setBorder(selected, &border);

    PaletteStageModule* paletteModule
        = stageModule<PaletteStageModule>(pipeline, VisualPlan::PaletteStage);
    if (paletteModule != 0)
        paletteModule->setPalette(
            (selected != 0) ? &selected->palette : 0,
            (selected != 0) ? &selected->currentPalette : 0,
            (selected != 0) ? &selected->palChanged : 0,
            (selected != 0) ? &selected->lastPalette : 0);
}

void VisualDirector::configurePipeline(VisualPipeline& pipeline) {
    bindPipelineStages(pipeline);

    pipeline.setStageMode(VisualPlan::BufferFrameBeginStage, VisualStageEnabled);
    if (pcxSelectionChanged())
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
