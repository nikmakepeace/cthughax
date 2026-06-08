/** @file
 * Source-level boundary checks for the Frame Generator module.
 */

#include <assert.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>

#ifndef CTHUGHA_SOURCE_DIR
#error CTHUGHA_SOURCE_DIR must point at the repository root
#endif

static std::string readSourceFile(const char* relativePath) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    assert(file.good());

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

static int sourceExists(const char* relativePath) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    return file.good();
}

static void assertSourceDoesNotExist(const char* relativePath) {
    if (sourceExists(relativePath))
        fprintf(stderr, "%s still exists\n", relativePath);
    assert(!sourceExists(relativePath));
}

static void assertSourceContains(const char* relativePath, const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) == std::string::npos)
        fprintf(stderr, "%s does not contain `%s`\n", relativePath, token);
    assert(contents.find(token) != std::string::npos);
}

static void assertSourceDoesNotContain(const char* relativePath,
    const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) != std::string::npos)
        fprintf(stderr, "%s still contains `%s`\n", relativePath, token);
    assert(contents.find(token) == std::string::npos);
}

static void assertFilesDoNotContain(const char* const* files, int fileCount,
    const char* token) {
    for (int i = 0; i < fileCount; ++i)
        assertSourceDoesNotContain(files[i], token);
}

static void testFrameGeneratorOwnsApplicationStorageAndPipeline() {
    assertSourceContains("src/Application.h",
        "FrameGeneratorRuntime frameGeneratorValue");
    assertSourceContains("src/Application.cc",
        "frameGeneratorValue.geometry().maxDimension()");
    assertSourceContains("src/Application.cc", "frameGeneratorValue.render");
    assertSourceDoesNotContain("src/Application.cc", "CthughaBuffer");
    assertSourceDoesNotContain("src/Application.h", "CthughaBuffer");
    assertSourceDoesNotContain("src/Application.cc", "FrameRenderTarget");
    assertSourceDoesNotContain("src/Application.h", "FrameRenderTarget");
    assertSourceDoesNotContain("src/Application.h", "VideoDirector");
    assertSourceDoesNotContain("src/Application.cc", "VideoDirector");
    assertSourceDoesNotContain("src/Application.h", "videoFilterchain");
    assertSourceDoesNotContain("src/Application.cc", "videoFilterchain");
}

static void testLegacyCthughaBufferIsRetired() {
    assertSourceDoesNotExist("src/CthughaBuffer.h");
    assertSourceDoesNotExist("src/CthughaBuffer.cc");
    assertSourceDoesNotContain("src/FrameRenderTarget.h", "static ");
    assertSourceDoesNotContain("src/FrameRenderTarget.cc",
        "FrameRenderTarget::buffer");
    assertSourceDoesNotContain("src/FrameRenderTarget.cc",
        "FrameRenderTarget::current");
}

static void testOldVideoDirectorIsRetired() {
    assertSourceDoesNotExist("src/VideoDirector.h");
    assertSourceDoesNotExist("src/VideoDirector.cc");
}

static void testLegacyVideoFilterNamesAreRetired() {
    assertSourceDoesNotExist("src/VideoFilterchain.h");
    assertSourceDoesNotExist("src/VideoFilterchain.cc");
    assertSourceDoesNotExist("src/VideoFilterchainFactory.h");
    assertSourceDoesNotExist("src/VideoFilterchainFactory.cc");
    assertSourceDoesNotExist("src/VideoFilterchainSequence.h");
    assertSourceDoesNotExist("src/VideoFilterchainSequence.cc");
    assertSourceDoesNotExist("src/VideoFilters.h");
    assertSourceDoesNotExist("src/VideoFilters.cc");
    assertSourceDoesNotExist("src/VideoFrameBudget.h");
    assertSourceDoesNotExist("src/VideoFrameBudget.cc");
}

static void testFrameStoreOwnsStorageLayout() {
    assertSourceContains("src/FrameStore.h", "FrameStorageLayout layoutValue");
    assertSourceContains("src/FrameStore.cc", "bufferValue.setLayout(layoutValue)");
    assertSourceContains("src/FrameRenderTarget.h", "int pitch() const");
    assertSourceContains("src/FrameRenderTarget.h", "int visibleOffset(int x, int y) const");
    assertSourceContains("src/FrameStorageLayout.h", "int visibleLinearOffset");
    assertSourceDoesNotContain("src/FrameGeometry.h", "hiddenBorder");
    assertSourceDoesNotContain("src/FrameGeometry.h", "int pitch");
    assertSourceDoesNotContain("src/FrameGeometry.h", "pitchValue");
    assertSourceDoesNotContain("src/FrameStore.cc",
        "geometryValue.width());");
}

static void testDisplayNeverReadsGeneratorCurrentStorage() {
    assertSourceDoesNotContain("src/CthughaDisplay.cc",
        "FrameRenderTarget::current");
    assertSourceDoesNotContain("src/CthughaDisplay.cc",
        "#include \"FrameRenderTarget.h\"");
    assertSourceDoesNotContain("src/CthughaDisplay.h", "presentCurrent");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "presentCurrent");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "FrameRenderTarget::current");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "#include \"FrameRenderTarget.h\"");
}

static void testFrameGeneratorModuleDoesNotReachDisplayOrRuntimeCommands() {
    static const char* const files[] = {
        "src/FrameGeneratorPipeline.h",
        "src/FrameGeneratorPipeline.cc",
        "src/FrameGeneratorContext.h",
        "src/FrameGeneratorContext.cc",
        "src/FrameGeneratorRuntime.h",
        "src/FrameGeneratorRuntime.cc",
        "src/FrameGeneratorSceneBinding.h",
        "src/FrameGeneratorSceneBinding.cc",
        "src/AudioAnalysisSnapshot.h",
        "src/AudioAnalysisSnapshot.cc",
        "src/FrameGeometry.h",
        "src/FrameGeometry.cc",
        "src/FrameRenderTarget.h",
        "src/FrameRenderTarget.cc",
        "src/FrameStore.h",
        "src/FrameStore.cc",
        "src/FrameStorageLayout.h",
        "src/FrameTransitionController.h",
        "src/FrameTransitionController.cc",
        "src/FrameGeneratorFrameBudget.h",
        "src/FrameGeneratorFrameBudget.cc",
        "src/FrameFilterchain.h",
        "src/FrameFilterchain.cc",
        "src/FrameFilterchainFactory.h",
        "src/FrameFilterchainFactory.cc",
        "src/FrameFilterchainSequence.h",
        "src/FrameFilterchainSequence.cc",
        "src/FrameFilters.h",
        "src/FrameFilters.cc",
        "src/Translate.h",
        "src/Translate.cc",
        "src/Wave.h",
        "src/Wave.cc",
        "src/waves.cc"
    };
    static const int fileCount = sizeof(files) / sizeof(files[0]);

    assertFilesDoNotContain(files, fileCount, "cthughaDisplay");
    assertFilesDoNotContain(files, fileCount, "displayDevice");
    assertFilesDoNotContain(files, fileCount, "displayRuntime");
    assertFilesDoNotContain(files, fileCount, "DisplayRuntime");
    assertFilesDoNotContain(files, fileCount, "RuntimeCommand");
    assertFilesDoNotContain(files, fileCount, "RuntimePersistence");
    assertFilesDoNotContain(files, fileCount, "CthughaBuffer");
    assertFilesDoNotContain(files, fileCount, "FrameRenderTarget::buffer");
    assertFilesDoNotContain(files, fileCount, "FrameRenderTarget::current");
    assertFilesDoNotContain(files, fileCount, "VideoFilterchain");
    assertFilesDoNotContain(files, fileCount, "VideoFilters");
    assertFilesDoNotContain(files, fileCount, "VideoFrameContext");
    assertFilesDoNotContain(files, fileCount, "VideoFrameBudget");
    assertFilesDoNotContain(files, fileCount, "#include \"CthughaDisplay.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"DisplayDevice.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"DisplayRuntime.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"display.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"Border.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"Flashlight.h\"");
    assertSourceDoesNotContain("src/FrameGeneratorRuntime.cc",
        "#include \"Configuration.h\"");
    assertSourceDoesNotContain("src/FrameGeneratorRuntime.h",
        "DisplayConfig");
    assertSourceDoesNotContain("src/FrameGeneratorRuntime.h",
        "MessagesConfig");
    assertSourceDoesNotContain("src/FrameGeneratorRuntime.h",
        "SceneTransitionPolicy");
    assertSourceDoesNotContain("src/FrameGeometry.cc",
        "#include \"Configuration.h\"");
    assertSourceDoesNotContain("src/FrameTransitionController.cc",
        "#include \"Configuration.h\"");
    assertSourceDoesNotContain("src/SilenceMessage.cc",
        "#include \"Configuration.h\"");
    assertSourceDoesNotContain("src/QotdMessagesProvider.cc",
        "#include \"Configuration.h\"");
    assertSourceContains("src/FrameFilters.cc", "#include \"BorderRenderer.h\"");
    assertSourceContains("src/FrameFilters.cc",
        "#include \"FlashlightRenderer.h\"");
    assertSourceDoesNotContain("src/BorderRenderer.h", "EffectControl");
    assertSourceDoesNotContain("src/FlashlightRenderer.h", "EffectControl");
    assertSourceContains("src/FrameGeneratorSceneBinding.h",
        "#include \"ImagePlacement.h\"");
    assertSourceContains("src/FrameFilters.h", "#include \"ImagePlacement.h\"");
    assertSourceDoesNotContain("src/FrameGeneratorRuntime.h", "ImageOption");
    assertSourceDoesNotContain("src/FrameGeneratorRuntime.h", "Option");
    assertSourceDoesNotContain("src/FrameGeneratorSceneBinding.h", "ImageOption");
    assertSourceDoesNotContain("src/FrameTransitionController.h", "Option");
    assertSourceDoesNotContain("src/FrameTransitionController.cc", "Option");
    assertSourceDoesNotContain("src/FrameGeneratorSceneBinding.h",
        "#include \"Image.h\"");
    assertSourceDoesNotContain("src/FrameFilters.h", "#include \"Image.h\"");
    assertSourceContains("src/FrameGeneratorSceneBinding.cc",
        "#include \"IndexedImage.h\"");
    assertSourceDoesNotContain("src/FrameGeneratorSceneBinding.cc",
        "#include \"Image.h\"");
    assertSourceContains("src/FrameFilters.cc", "#include \"IndexedImage.h\"");
    assertSourceDoesNotContain("src/FrameFilters.cc", "#include \"Image.h\"");
    assertSourceDoesNotContain("src/IndexedImage.h", "EffectControl");
    assertSourceContains("src/Image.h", "#include \"IndexedImage.h\"");
}

static void testGeneratorDiagnosticsAndMathTablesAreOwned() {
    static const char* const files[] = {
        "src/FrameGeneratorPipeline.h",
        "src/FrameGeneratorPipeline.cc",
        "src/FrameGeneratorRuntime.h",
        "src/FrameGeneratorRuntime.cc",
        "src/FrameGeneratorSceneBinding.h",
        "src/FrameGeneratorSceneBinding.cc",
        "src/FrameFilterchain.h",
        "src/FrameFilterchain.cc",
        "src/FrameFilterchainFactory.h",
        "src/FrameFilterchainFactory.cc",
        "src/FrameFilters.h",
        "src/FrameFilters.cc",
        "src/FrameRenderTarget.h",
        "src/FrameRenderTarget.cc",
        "src/Translate.h",
        "src/Translate.cc",
        "src/Wave.h",
        "src/Wave.cc",
        "src/waves.cc"
    };
    static const int fileCount = sizeof(files) / sizeof(files[0]);

    assertSourceContains("src/FrameGeneratorRuntime.h", "LogSink& logValue");
    assertSourceContains("src/FrameGeneratorPipeline.h",
        "explicit FrameGeneratorPipeline(LogSink& log)");
    assertSourceContains("src/FrameFilterchain.h",
        "explicit FrameFilterchain(LogSink& log)");
    assertSourceContains("src/FrameFilterchainFactory.h",
        "explicit FrameFilterchainFactory(LogSink& log)");
    assertSourceContains("src/FrameFilterchain.h", "LogSink& log() const");
    assertSourceContains("src/Wave.h", "LogSink& log() const");

    assertFilesDoNotContain(files, fileCount, "CTH_DEBUG");
    assertFilesDoNotContain(files, fileCount, "CTH_INFO");
    assertFilesDoNotContain(files, fileCount, "CTH_WARN");
    assertFilesDoNotContain(files, fileCount, "CTH_ERROR");
    assertFilesDoNotContain(files, fileCount, "CTH_TRACE");
    assertFilesDoNotContain(files, fileCount, "CTH_LOG_ENABLED");
    assertFilesDoNotContain(files, fileCount, "#include \"cthugha.h\"");
    assertSourceDoesNotContain("src/FrameFilters.cc",
        "static int debugReports");
    assertSourceDoesNotContain("src/waves.cc", "isin(");
    assertSourceDoesNotContain("src/waves.cc", "icos(");
    assertSourceDoesNotContain("src/waves.cc", "sine[");
    assertSourceDoesNotContain("src/imath.h", "init_imath");
    assertSourceDoesNotContain("src/imath.h", "extern int sine");
    assertSourceDoesNotContain("src/imath.h", "isin(");
}

static void testFrameGeneratorUsesNativeAudioSnapshot() {
    static const char* const files[] = {
        "src/FrameGeneratorPipeline.h",
        "src/FrameGeneratorPipeline.cc",
        "src/FrameGeneratorContext.h",
        "src/FrameGeneratorContext.cc",
        "src/FrameGeneratorRuntime.h",
        "src/FrameGeneratorRuntime.cc",
        "src/FrameFilterchain.h",
        "src/FrameFilterchain.cc",
        "src/FrameFilters.h",
        "src/FrameFilters.cc",
        "src/AudioAnalysisSnapshot.h",
        "src/AudioAnalysisSnapshot.cc",
        "src/Border.h",
        "src/Border.cc",
        "src/Flashlight.h",
        "src/Flashlight.cc",
        "src/Flame.h",
        "src/Flame.cc",
        "src/flames.cc",
        "src/Translate.h",
        "src/Translate.cc",
        "src/Wave.h",
        "src/Wave.cc",
        "src/waves.cc"
    };
    static const int fileCount = sizeof(files) / sizeof(files[0]);

    assertSourceContains("src/FrameGeneratorContext.h",
        "AudioAnalysisSnapshot audioAnalysisValue");
    assertSourceContains("src/FrameGeneratorContext.h",
        "const AudioAnalysisSnapshot& audioAnalysis() const");
    assertSourceContains("src/Application.cc",
        "AudioAnalysisSnapshot audioAnalysis(frame.metrics");

    assertFilesDoNotContain(files, fileCount, "FrameRenderContext");
    assertFilesDoNotContain(files, fileCount, "AcousticContext");
    assertFilesDoNotContain(files, fileCount, "AudioAnalyzer.h");
}

int main() {
    testFrameGeneratorOwnsApplicationStorageAndPipeline();
    testLegacyCthughaBufferIsRetired();
    testOldVideoDirectorIsRetired();
    testLegacyVideoFilterNamesAreRetired();
    testFrameStoreOwnsStorageLayout();
    testDisplayNeverReadsGeneratorCurrentStorage();
    testFrameGeneratorModuleDoesNotReachDisplayOrRuntimeCommands();
    testGeneratorDiagnosticsAndMathTablesAreOwned();
    testFrameGeneratorUsesNativeAudioSnapshot();
    return 0;
}
