#include "AudioAnalyzer.h"
#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"
#include "Screen.h"
#include "ScreenRenderContext.h"

#include <benchmark/benchmark.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

const int kDefaultWidth = 1600;
const int kDefaultHeight = 1200;
const int kMaxBenchmarkDimension = 4096;

struct DisplayScreensBenchConfig {
    int width;
    int height;
    std::string patternList;

    DisplayScreensBenchConfig()
        : width(kDefaultWidth)
        , height(kDefaultHeight)
        , patternList("zero,gradient,noise") { }
};

DisplayScreensBenchConfig& config() {
    static DisplayScreensBenchConfig value;
    return value;
}

int visiblePixels() {
    return config().width * config().height;
}

void failConfiguration(const std::string& message) {
    fprintf(stderr, "display_screens_bench: %s\n", message.c_str());
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
        || parsedWidth > kMaxBenchmarkDimension
        || parsedHeight > kMaxBenchmarkDimension)
        return 0;

    width = (int)parsedWidth;
    height = (int)parsedHeight;
    return 1;
}

void parseDisplayScreensArgs(int* argc, char** argv) {
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
        } else if (strncmp(arg, "--cth-patterns=", 15) == 0) {
            config().patternList = arg + 15;
        } else {
            argv[output++] = argv[i];
        }
    }

    *argc = output;
}

std::vector<std::string> patternNames() {
    std::vector<std::string> names;
    std::stringstream ss(config().patternList);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty())
            names.push_back(item);
    }

    if (names.empty())
        names.push_back("zero");

    return names;
}

struct SourceFixture {
    std::string name;
    std::vector<unsigned char> pixels;

    IndexedFrame frame() const {
        return IndexedFrame(pixels.data(), config().width, config().height,
            config().width, 0);
    }
};

void fillPattern(SourceFixture& fixture) {
    fixture.pixels.assign(visiblePixels(), 0);

    if (fixture.name == "zero")
        return;

    if (fixture.name == "gradient") {
        int widthDivisor = config().width > 1 ? config().width - 1 : 1;
        int heightDivisor = config().height > 1 ? config().height - 1 : 1;
        for (int y = 0; y < config().height; y++) {
            for (int x = 0; x < config().width; x++) {
                int xValue = (x * 255) / widthDivisor;
                int yValue = (y * 255) / heightDivisor;
                fixture.pixels[y * config().width + x]
                    = (unsigned char)((xValue + yValue) / 2);
            }
        }
        return;
    }

    if (fixture.name == "checker") {
        for (int y = 0; y < config().height; y++) {
            for (int x = 0; x < config().width; x++) {
                int tile = ((x / 16) + (y / 16)) & 1;
                fixture.pixels[y * config().width + x]
                    = (unsigned char)(tile ? 224 : 32);
            }
        }
        return;
    }

    if (fixture.name == "noise") {
        uint32_t value = 0x12345678u;
        for (int i = 0; i < visiblePixels(); i++) {
            value = value * 1664525u + 1013904223u;
            fixture.pixels[i] = (unsigned char)(value >> 24);
        }
        return;
    }

    failConfiguration("unknown --cth-patterns entry: " + fixture.name);
}

std::vector<SourceFixture> loadFixtures() {
    std::vector<std::string> names = patternNames();
    std::vector<SourceFixture> fixtures;
    fixtures.reserve(names.size());

    for (size_t i = 0; i < names.size(); i++) {
        SourceFixture fixture;
        fixture.name = names[i];
        fillPattern(fixture);
        fixtures.push_back(fixture);
    }

    return fixtures;
}

const std::vector<SourceFixture>& fixtures() {
    static std::vector<SourceFixture> value = loadFixtures();
    return value;
}

void initializeDestination(IndexedDisplayFrame& destination, ScreenEntry& entry) {
    xy output = entry.outputSize(config().width, config().height);
    destination.resize(output.x, output.y);
    if (!destination.valid())
        failConfiguration("screen output dimensions are invalid");

    memset(destination.pixels(), 0, destination.byteCount());
}

void setCommonCounters(benchmark::State& state,
    const IndexedDisplayFrame& destination) {
    state.SetItemsProcessed((int64_t)state.iterations() * visiblePixels());
    state.SetBytesProcessed((int64_t)state.iterations()
        * (int64_t)(visiblePixels() + destination.byteCount()));
}

static void BM_Screen(benchmark::State& state, ScreenEntry* entry,
    const SourceFixture* fixture) {
    IndexedDisplayFrame destination;
    initializeDestination(destination, *entry);

    IndexedFrame source = fixture->frame();
    double frameTime = 0.0;
    const double deltaTime = 1.0 / 60.0;
    int result = 0;

    for (auto _ : state) {
        ScreenRenderContext context(source, destination, frameTime, deltaTime,
            60.0);
        result = entry->render(context);
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(destination.pixels()[0]);
        benchmark::ClobberMemory();
        frameTime += deltaTime;
    }

    if (result != 0)
        state.SkipWithError("screen renderer rejected the benchmark geometry");

    setCommonCounters(state, destination);
}

std::string benchmarkName(const char* entryName, const std::string& patternName) {
    std::string name("Screen/");
    name += entryName;
    name += "/";
    name += patternName;
    return name;
}

void registerDisplayScreenBenchmarks() {
    const std::vector<SourceFixture>& allFixtures = fixtures();

    for (int i = 0; i < nScreenCatalogEntries; i++) {
        ScreenEntry* entry = screenByIndex(i);
        for (size_t f = 0; f < allFixtures.size(); f++) {
            benchmark::RegisterBenchmark(
                benchmarkName(entry->Name(), allFixtures[f].name).c_str(),
                &BM_Screen, entry, &allFixtures[f])
                ->Unit(benchmark::kMicrosecond);
        }
    }
}

} // namespace

char* cthugha_mode_text() {
    return (char*)"";
}

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_context(int, const char*, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

int cth_log_errno(int, const char*, ...) {
    return 0;
}

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0) {
}

AcousticContext::AcousticContext(LogSink*)
    : log(0)
    , intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , cumulativeFireLevelValue(0) {
}

void AcousticContext::update(const AudioMetrics&) {
}

double AcousticContext::intensity() const {
    return 0.0;
}

int AcousticContext::fire() const {
    return 0;
}

int AcousticContext::cumulativeFireLevel() const {
    return 0;
}

void AcousticContext::resetCumulativeFireLevel() {
}

int main(int argc, char** argv) {
    parseDisplayScreensArgs(&argc, argv);

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    registerDisplayScreenBenchmarks();
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
