#include "FrameRenderTarget.h"
#include "CthughaDisplay.h"
#include "FrameStore.h"
#include "FrameStageBuffer.h"
#include "Flame.h"
#include "Translate.h"
#include "FrameGeneratorContext.h"
#include "imath.h"
#include "TranslationOptions.h"
#include "cth_buffer.h"

#include <benchmark/benchmark.h>

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

EffectChoiceList paletteEntries;
EffectChoiceList pcxEntries;

struct PathConfig;
class LogSink;

int init_wave(const PathConfig&, LogSink&) { return 0; }
int load_palettes(const PathConfig&) { return 0; }
int init_pcx() { return 0; }

CthughaDisplay* cthughaDisplay = NULL;

namespace {

const int kHiddenRows = 3;
const int kMaxBenchmarkDimension = 4096;

struct FrameEffectsBenchConfig {
    int width;
    int height;
    std::string fixtureDir;
    std::string fixtureList;
    std::string destinationImagePath;
    std::string sourceImagePath;

    FrameEffectsBenchConfig()
        : width(1600)
        , height(1200) { }
};

FrameEffectsBenchConfig& config() {
    static FrameEffectsBenchConfig value;
    return value;
}

int fullHeight() {
    return config().height + 2 * kHiddenRows;
}

int visiblePixels() {
    return config().width * config().height;
}

int fullPixels() {
    return config().width * fullHeight();
}

void failConfiguration(const std::string& message) {
    fprintf(stderr, "visual_effects_bench: %s\n", message.c_str());
    exit(2);
}

int parseSize(const char* value, int& width, int& height) {
    char* end = NULL;
    long parsedWidth = strtol(value, &end, 10);
    if (end == value || (*end != 'x' && *end != 'X'))
        return 0;

    char* heightStart = end + 1;
    long parsedHeight = strtol(heightStart, &end, 10);
    if (end == heightStart || *end != '\0')
        return 0;

    if (parsedWidth <= 0 || parsedHeight <= 0
        || parsedWidth > kMaxBenchmarkDimension || parsedHeight > kMaxBenchmarkDimension)
        return 0;

    width = (int)parsedWidth;
    height = (int)parsedHeight;
    return 1;
}

void parseFrameEffectsArgs(int* argc, char** argv) {
    int output = 1;

    for (int i = 1; i < *argc; i++) {
        const char* arg = argv[i];

        if (strncmp(arg, "--cth-buffer-size=", 18) == 0) {
            int width = 0;
            int height = 0;
            if (!parseSize(arg + 18, width, height))
                failConfiguration("expected --cth-buffer-size=WIDTHxHEIGHT");
            config().width = width;
            config().height = height;
        } else if (strncmp(arg, "--cth-fixture-dir=", 18) == 0) {
            config().fixtureDir = arg + 18;
        } else if (strncmp(arg, "--cth-fixtures=", 15) == 0) {
            config().fixtureList = arg + 15;
        } else if (strncmp(arg, "--cth-destination=", 18) == 0) {
            config().destinationImagePath = arg + 18;
        } else if (strncmp(arg, "--cth-source=", 13) == 0) {
            config().sourceImagePath = arg + 13;
        } else {
            argv[output++] = argv[i];
        }
    }

    *argc = output;

    if (config().destinationImagePath.empty() != config().sourceImagePath.empty())
        failConfiguration("--cth-destination and --cth-source must be specified together");
}

struct ImageFixture {
    std::string name;
    int width;
    int height;
    std::vector<unsigned char> pixels;
    int loaded;

    ImageFixture()
        : width(0)
        , height(0)
        , loaded(0) { }
};

struct BufferFixture {
    std::string name;
    ImageFixture base;
    ImageFixture destination;
    ImageFixture source;
};

std::vector<TranslateEntry*> translateEntries;

std::string videoFixtureDir() {
    if (!config().fixtureDir.empty())
        return config().fixtureDir;

    const char* env = getenv("CTH_VISUAL_FIXTURE_DIR");
    if (env != NULL && env[0] != '\0')
        return env;
    return CTH_VISUAL_FIXTURE_DIR;
}

int fileExists(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    return file.good();
}

std::string fixturePath(const std::string& stem, const char* extension) {
    return videoFixtureDir() + "/" + stem + extension;
}

int readPgmToken(std::istream& in, std::string& token) {
    token.clear();

    while (in.good()) {
        int c = in.peek();
        if (isspace(c)) {
            in.get();
            continue;
        }
        if (c == '#') {
            std::string ignored;
            std::getline(in, ignored);
            continue;
        }
        break;
    }

    while (in.good()) {
        int c = in.peek();
        if (isspace(c) || c == '#')
            break;
        token.push_back((char)in.get());
    }

    return token.empty() ? 0 : 1;
}

int loadPgm(const std::string& path, ImageFixture& image) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.good())
        return 0;

    std::string token;
    if (!readPgmToken(in, token) || token != "P5")
        return 0;

    if (!readPgmToken(in, token))
        return 0;
    int width = atoi(token.c_str());
    if (!readPgmToken(in, token))
        return 0;
    int height = atoi(token.c_str());
    if (!readPgmToken(in, token))
        return 0;
    int maxValue = atoi(token.c_str());

    if (width != config().width
        || (height != config().height && height != fullHeight())
        || maxValue <= 0 || maxValue > 255)
        failConfiguration(path + " does not match --cth-buffer-size; expected "
            + std::to_string(config().width) + "x" + std::to_string(config().height)
            + " or " + std::to_string(config().width) + "x" + std::to_string(fullHeight()));

    if (in.peek() != EOF && isspace(in.peek()))
        in.get();

    image.width = width;
    image.height = height;
    image.pixels.resize(width * height);
    in.read((char*)image.pixels.data(), (std::streamsize)image.pixels.size());

    if ((size_t)in.gcount() != image.pixels.size())
        return 0;

    if (maxValue != 255) {
        for (size_t i = 0; i < image.pixels.size(); i++)
            image.pixels[i] = (unsigned char)(((int)image.pixels[i] * 255) / maxValue);
    }

    image.loaded = 1;
    return 1;
}

int loadRaw(const std::string& path, ImageFixture& image) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.good())
        return 0;

    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());

    if ((int)data.size() != visiblePixels() && (int)data.size() != fullPixels())
        failConfiguration(path + " does not match --cth-buffer-size; expected "
            + std::to_string(visiblePixels()) + " or "
            + std::to_string(fullPixels()) + " raw bytes");

    image.width = config().width;
    image.height = ((int)data.size() == fullPixels()) ? fullHeight() : config().height;
    image.pixels.swap(data);
    image.loaded = 1;
    return 1;
}

int loadImagePath(const std::string& path, ImageFixture& image) {
    image.name = path;
    if (!fileExists(path))
        failConfiguration("fixture image not found: " + path);

    std::ifstream in(path.c_str(), std::ios::binary);
    char magic[2] = { 0, 0 };
    in.read(magic, 2);
    in.close();

    if (magic[0] == 'P' && magic[1] == '5')
        return loadPgm(path, image);

    return loadRaw(path, image);
}

int loadImageStem(const std::string& stem, ImageFixture& image) {
    image.name = stem;

    std::string pgmPath = fixturePath(stem, ".pgm");
    if (fileExists(pgmPath))
        return loadPgm(pgmPath, image);

    std::string rawPath = fixturePath(stem, ".raw");
    if (fileExists(rawPath))
        return loadRaw(rawPath, image);

    return 0;
}

void generateFixture(const std::string& name, ImageFixture& image) {
    image.name = name;
    image.width = config().width;
    image.height = config().height;
    image.pixels.assign(visiblePixels(), 0);
    image.loaded = 0;

    if (name == "impulse") {
        int cx = config().width / 2;
        int cy = config().height / 2;
        image.pixels[(cy * config().width) + cx] = 255;
        if (cy + 1 < config().height)
            image.pixels[((cy + 1) * config().width) + cx] = 192;
        if (cx + 1 < config().width)
            image.pixels[(cy * config().width) + cx + 1] = 192;
    } else if (name == "gradient") {
        int divisor = (config().width > 1) ? config().width - 1 : 1;
        for (int y = 0; y < config().height; y++) {
            for (int x = 0; x < config().width; x++)
                image.pixels[y * config().width + x] = (unsigned char)((x * 255) / divisor);
        }
    } else if (name == "noise") {
        uint32_t value = 0x12345678u;
        for (int i = 0; i < visiblePixels(); i++) {
            value = value * 1664525u + 1013904223u;
            image.pixels[i] = (unsigned char)(value >> 24);
        }
    }
}

ImageFixture loadOrGenerateImage(const std::string& stem) {
    ImageFixture image;
    if (!loadImageStem(stem, image))
        generateFixture(stem, image);
    return image;
}

std::vector<std::string> fixtureNames() {
    if (!config().fixtureList.empty()) {
        std::vector<std::string> names;
        std::stringstream ss(config().fixtureList);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty())
                names.push_back(item);
        }
        if (!names.empty())
            return names;
    }

    const char* env = getenv("CTH_VISUAL_FIXTURES");
    if (env == NULL || env[0] == '\0')
        return { "zero", "impulse", "gradient", "noise" };

    std::vector<std::string> names;
    std::stringstream ss(env);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty())
            names.push_back(item);
    }
    if (names.empty())
        names.push_back("zero");
    return names;
}

std::vector<BufferFixture> loadFixtures() {
    std::vector<BufferFixture> result;

    if (!config().destinationImagePath.empty()) {
        BufferFixture fixture;
        fixture.name = "custom";
        loadImagePath(config().destinationImagePath, fixture.destination);
        loadImagePath(config().sourceImagePath, fixture.source);
        fixture.base = fixture.source;
        result.push_back(fixture);
        return result;
    }

    std::vector<std::string> names = fixtureNames();

    for (size_t i = 0; i < names.size(); i++) {
        BufferFixture fixture;
        fixture.name = names[i];
        fixture.base = loadOrGenerateImage(names[i]);
        loadImageStem(names[i] + "-destination", fixture.destination);
        loadImageStem(names[i] + "-source", fixture.source);
        result.push_back(fixture);
    }

    return result;
}

const std::vector<BufferFixture>& fixtures() {
    static std::vector<BufferFixture> all = loadFixtures();
    return all;
}

const ImageFixture& zeroImage() {
    static ImageFixture image = loadOrGenerateImage("zero");
    return image;
}

FrameStore& benchmarkStore() {
    static FrameStore store;
    return store;
}

FrameRenderTarget& benchmarkBuffer() {
    return benchmarkStore().renderTarget();
}

void copyImageWithHiddenRows(unsigned char* visibleBuffer, const ImageFixture& image) {
    unsigned char* fullBuffer = visibleBuffer - kHiddenRows * config().width;
    memset(fullBuffer, 0, fullPixels());

    if (image.width != config().width || image.pixels.empty())
        return;

    if (image.height == fullHeight())
        memcpy(fullBuffer, image.pixels.data(), fullPixels());
    else
        memcpy(visibleBuffer, image.pixels.data(), visiblePixels());
}

void resetForFlame(const BufferFixture& fixture) {
    FrameRenderTarget& buffer = benchmarkBuffer();
    const ImageFixture& source =
        fixture.source.pixels.empty() ? fixture.base : fixture.source;
    copyImageWithHiddenRows(buffer.sourcePixels(), source);
    copyImageWithHiddenRows(buffer.destinationPixels(),
        fixture.destination.pixels.empty() ? source : fixture.destination);
}

void resetForTranslate(const BufferFixture& fixture) {
    FrameRenderTarget& buffer = benchmarkBuffer();
    copyImageWithHiddenRows(buffer.sourcePixels(),
        fixture.source.pixels.empty() ? fixture.base : fixture.source);
    copyImageWithHiddenRows(buffer.destinationPixels(),
        fixture.destination.pixels.empty() ? zeroImage() : fixture.destination);
}

void createTranslationEntries() {
    if (!translateEntries.empty())
        return;

    const char* names[] = { "identity", "mirror-x", "mirror-y", "random" };

    for (size_t s = 0; s < sizeof(names) / sizeof(names[0]); s++) {
        std::vector<int> table(visiblePixels());

        uint32_t random = 0x9e3779b9u;
        for (int y = 0; y < config().height; y++) {
            for (int x = 0; x < config().width; x++) {
                int index = y * config().width + x;
                if (strcmp(names[s], "mirror-x") == 0)
                    table[index] = y * config().width + (config().width - 1 - x);
                else if (strcmp(names[s], "mirror-y") == 0)
                    table[index] = (config().height - 1 - y) * config().width + x;
                else if (strcmp(names[s], "random") == 0) {
                    random = random * 1664525u + 1013904223u;
                    table[index] = (int)(random % visiblePixels());
                } else
                    table[index] = index;
            }
        }

        TranslateEntry* entry = new TranslateEntry(names[s], names[s],
            table, config().width, config().height);
        translateEntries.push_back(entry);
    }
}

void initializeFrameEffectsBenchmarks() {
    static int initialized = 0;
    if (initialized)
        return;

    initialized = 1;
    benchmarkStore().resize(FrameStorageLayout(PixelSize(config().width,
        config().height), config().width, kHiddenRows));

    srand(0);
    init_flames();
    createTranslationEntries();
}

void setCommonCounters(benchmark::State& state) {
    state.SetItemsProcessed((int64_t)state.iterations() * visiblePixels());
    state.SetBytesProcessed((int64_t)state.iterations() * visiblePixels());
}

static void BM_Flame(benchmark::State& state, const Flame* flame,
    const BufferFixture* fixture) {
    initializeFrameEffectsBenchmarks();

    FrameGeneratorContext context;
    static FlameLookupTables lookupTables;
    FrameRenderTarget& buffer = benchmarkBuffer();
    FrameStageBuffer stageBuffer(buffer);

    for (auto _ : state) {
        state.PauseTiming();
        resetForFlame(*fixture);
        state.ResumeTiming();

        flame->execute(stageBuffer, context, 0, lookupTables);
        benchmark::DoNotOptimize(buffer.destinationPixels()[0]);
        benchmark::ClobberMemory();
    }

    setCommonCounters(state);
}

static void BM_TranslateEntry(benchmark::State& state, TranslateEntry* entry,
    const BufferFixture* fixture) {
    initializeFrameEffectsBenchmarks();

    FrameGeneratorContext context;
    Translate translate(entry->table());
    FrameRenderTarget& buffer = benchmarkBuffer();
    FrameStageBuffer stageBuffer(buffer);

    for (auto _ : state) {
        state.PauseTiming();
        resetForTranslate(*fixture);
        state.ResumeTiming();

        translate.execute(stageBuffer, context);
        benchmark::DoNotOptimize(buffer.destinationPixels()[0]);
        benchmark::ClobberMemory();
    }

    setCommonCounters(state);
}

std::string benchmarkName(const char* prefix, const char* entryName,
    const std::string& fixtureName) {
    std::string name(prefix);
    name += "/";
    name += entryName;
    name += "/";
    name += fixtureName;
    return name;
}

void registerFrameEffectsBenchmarks() {
    initializeFrameEffectsBenchmarks();
    const std::vector<BufferFixture>& allFixtures = fixtures();

    for (int i = 0; i < nFlameCatalogEntries; i++) {
        const Flame* flame = flameByIndex(i);
        for (size_t f = 0; f < allFixtures.size(); f++) {
            benchmark::RegisterBenchmark(
                benchmarkName("Flame", flame->name(), allFixtures[f].name).c_str(),
                &BM_Flame, flame, &allFixtures[f])
                ->Unit(benchmark::kMicrosecond);
        }
    }

    for (size_t i = 0; i < translateEntries.size(); i++) {
        TranslateEntry* translate = translateEntries[i];
        for (size_t f = 0; f < allFixtures.size(); f++) {
            benchmark::RegisterBenchmark(
                benchmarkName("Translate", translate->Name(), allFixtures[f].name).c_str(),
                &BM_TranslateEntry, translate, &allFixtures[f])
                ->Unit(benchmark::kMicrosecond);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    parseFrameEffectsArgs(&argc, argv);

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    registerFrameEffectsBenchmarks();
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
