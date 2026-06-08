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
    assertSourceDoesNotContain("src/Application.cc", "CthughaBuffer::buffer");
    assertSourceDoesNotContain("src/Application.cc", "CthughaBuffer::current");
    assertSourceDoesNotContain("src/Application.h", "VideoDirector");
    assertSourceDoesNotContain("src/Application.cc", "VideoDirector");
    assertSourceDoesNotContain("src/Application.h", "videoFilterchain");
    assertSourceDoesNotContain("src/Application.cc", "videoFilterchain");
}

static void testLegacyBufferAliasesAreGone() {
    assertSourceDoesNotContain("src/CthughaBuffer.h",
        "static CthughaBuffer buffer");
    assertSourceDoesNotContain("src/CthughaBuffer.h",
        "static CthughaBuffer* current");
    assertSourceDoesNotContain("src/CthughaBuffer.cc",
        "CthughaBuffer::buffer");
    assertSourceDoesNotContain("src/CthughaBuffer.cc",
        "CthughaBuffer::current");
}

static void testOldVideoDirectorIsRetired() {
    assertSourceDoesNotExist("src/VideoDirector.h");
    assertSourceDoesNotExist("src/VideoDirector.cc");
}

static void testDisplayNeverReadsGeneratorCurrentStorage() {
    assertSourceDoesNotContain("src/CthughaDisplay.cc",
        "CthughaBuffer::current");
    assertSourceDoesNotContain("src/CthughaDisplay.cc",
        "#include \"CthughaBuffer.h\"");
    assertSourceDoesNotContain("src/CthughaDisplay.h", "presentCurrent");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "presentCurrent");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "CthughaBuffer::current");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "#include \"CthughaBuffer.h\"");
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
        "src/FrameGeometry.h",
        "src/FrameGeometry.cc",
        "src/FrameStore.h",
        "src/FrameStore.cc",
        "src/FrameTransitionController.h",
        "src/FrameTransitionController.cc",
        "src/VideoFilterchain.h",
        "src/VideoFilterchain.cc",
        "src/VideoFilters.h",
        "src/VideoFilters.cc",
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
    assertFilesDoNotContain(files, fileCount, "CthughaBuffer::buffer");
    assertFilesDoNotContain(files, fileCount, "CthughaBuffer::current");
    assertFilesDoNotContain(files, fileCount, "#include \"CthughaDisplay.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"DisplayDevice.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"DisplayRuntime.h\"");
    assertFilesDoNotContain(files, fileCount, "#include \"display.h\"");
}

static void testGeneratorDiagnosticsAndMathTablesAreOwned() {
    assertSourceDoesNotContain("src/VideoFilters.cc",
        "static int debugReports");
    assertSourceDoesNotContain("src/waves.cc", "isin(");
    assertSourceDoesNotContain("src/waves.cc", "icos(");
    assertSourceDoesNotContain("src/waves.cc", "sine[");
    assertSourceDoesNotContain("src/imath.h", "init_imath");
    assertSourceDoesNotContain("src/imath.h", "extern int sine");
    assertSourceDoesNotContain("src/imath.h", "isin(");
}

int main() {
    testFrameGeneratorOwnsApplicationStorageAndPipeline();
    testLegacyBufferAliasesAreGone();
    testOldVideoDirectorIsRetired();
    testDisplayNeverReadsGeneratorCurrentStorage();
    testFrameGeneratorModuleDoesNotReachDisplayOrRuntimeCommands();
    testGeneratorDiagnosticsAndMathTablesAreOwned();
    return 0;
}
