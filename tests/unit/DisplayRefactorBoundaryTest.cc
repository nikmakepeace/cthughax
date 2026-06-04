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
}

int main() {
    testScreenRenderContextHasNoAmbientCurrentContext();
    testScreenDispatchUsesExplicitRenderContextOnly();
    testScreenRenderersDoNotReadDisplayGlobals();
    testGenericDisplayCoordinatorUsesOwnedDisplayStage();
    return 0;
}
