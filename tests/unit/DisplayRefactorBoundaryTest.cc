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

static void assertSourceDoesNotContain(const char* relativePath,
    const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) != std::string::npos)
        fprintf(stderr, "%s still contains `%s`\n", relativePath, token);
    assert(contents.find(token) == std::string::npos);
}

static void assertSourceContains(const char* relativePath,
    const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) == std::string::npos)
        fprintf(stderr, "%s does not contain `%s`\n", relativePath, token);
    assert(contents.find(token) != std::string::npos);
}

static void testScreenRenderContextHasNoAmbientCurrentContext() {
    assertSourceDoesNotContain("src/ScreenRenderContext.h",
        "currentScreenRenderContext");
    assertSourceDoesNotContain("src/ScreenRenderContext.cc",
        "currentScreenRenderContext");
    assertSourceDoesNotContain("src/ScreenRenderContext.h",
        "ScopedScreenRenderContext");
    assertSourceDoesNotContain("src/ScreenRenderContext.cc",
        "ScopedScreenRenderContext");
    assertSourceDoesNotContain("src/ScreenRenderContext.h",
        "requestScreenChange");
    assertSourceDoesNotContain("src/ScreenRenderContext.cc",
        "requestScreenChange");
}

static void testScreenDispatchUsesExplicitRenderContextOnly() {
    assertSourceDoesNotContain("src/Screen.cc", "currentScreenRenderContext");
    assertSourceDoesNotContain("src/Screen.cc", "ScopedScreenRenderContext");
}

static void testScreenRenderersDoNotReadDisplayGlobals() {
    assertSourceDoesNotContain("src/display.cc", "currentScreenRenderContext");
    assertSourceDoesNotContain("src/display.cc", "cthughaDisplay");
    assertSourceDoesNotContain("src/display.cc", "draw_size");
    assertSourceDoesNotContain("src/display.cc", "requestScreenChange");
}

static void testGenericDisplayCoordinatorUsesOwnedDisplayStage() {
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "displayDevice");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "disp_size");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "draw_size");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "cth_buffer.h");
}

static void testApplicationOwnsDisplaySystemRoot() {
    assertSourceContains("src/Application.h", "DisplaySystem displaySystemValue");
    assertSourceContains("src/Application.cc", "DisplayDriverRegistry displayDrivers");
    assertSourceContains("src/Application.cc",
        "displaySystemValue.open(displayDrivers, displayOpenRequest)");
    assertSourceDoesNotContain("src/Application.cc", "publishAliases");
    assertSourceDoesNotContain("src/Application.cc", "newDisplayDevice");
    assertSourceDoesNotContain("src/Application.cc", "cthughaDisplay");
}

static void testDisplayHeadersDoNotExportMutableAliases() {
    assertSourceDoesNotContain("src/DisplayDevice.h", "extern DisplayDevice*");
    assertSourceDoesNotContain("src/DisplayBackend.h", "extern DisplayBackend*");
    assertSourceDoesNotContain("src/DisplayRuntime.h", "extern DisplayRuntime*");
    assertSourceDoesNotContain("src/CthughaDisplay.h", "extern CthughaDisplay*");
    assertSourceDoesNotContain("src/DisplayRuntime.h", "DisplayRuntimeOwnership");
}

static void testInterfaceDoesNotRenderThroughDisplayGlobals() {
    assertSourceDoesNotContain("src/Interface.cc", "#include \"DisplayDevice.h\"");
    assertSourceDoesNotContain("src/Interface.cc", "#include \"CthughaDisplay.h\"");
    assertSourceDoesNotContain("src/InterfaceList.cc", "#include \"DisplayDevice.h\"");
    assertSourceDoesNotContain("src/InterfaceHelp.cc", "#include \"CthughaDisplay.h\"");
    assertSourceDoesNotContain("src/InterfaceCredits.cc", "text_size");
}

static void testDisplayDoesNotIncludeGlobalFrameBuffer() {
    assertSourceDoesNotContain("src/CthughaDisplayX11.cc", "cth_buffer.h");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "cth_buffer.h");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "CthughaBuffer::current");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "CthughaBuffer::current");
}

int main() {
    testScreenRenderContextHasNoAmbientCurrentContext();
    testScreenDispatchUsesExplicitRenderContextOnly();
    testScreenRenderersDoNotReadDisplayGlobals();
    testGenericDisplayCoordinatorUsesOwnedDisplayStage();
    testApplicationOwnsDisplaySystemRoot();
    testDisplayHeadersDoNotExportMutableAliases();
    testInterfaceDoesNotRenderThroughDisplayGlobals();
    testDisplayDoesNotIncludeGlobalFrameBuffer();
    return 0;
}
