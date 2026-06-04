#include "Configuration.h"

#include "configuration_defaults.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#ifndef CTH_XWIN
#error X11ConfigurationTest must be compiled only for the X11 frontend
#endif

static const std::string* patchValue(const ConfigPatch& patch,
    const std::string& key) {
    const std::string* value = patch.value(key);
    assert(value != NULL);
    return value;
}

static void defaultsProduceX11ConfigOnlyForX11Builds() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults().build();

    assert(result.ok());
    assert(result.config.x11.textOnTerm == X11_CONFIG_DEFAULT_TEXT_ON_TERM);
    assert(result.config.x11.overrideRedirect
        == X11_CONFIG_DEFAULT_OVERRIDE_REDIRECT);
    assert(result.config.x11.privateCmap == X11_CONFIG_DEFAULT_PRIVATE_CMAP);
    assert(result.config.x11.mitShm == X11_CONFIG_DEFAULT_MIT_SHM);
    assert(result.config.x11.rootWindow == X11_CONFIG_DEFAULT_ROOT_WINDOW);
    assert(result.config.x11.fullscreen == X11_CONFIG_DEFAULT_FULLSCREEN);
    assert(result.config.x11.windowPositionEnabled
        == X11_CONFIG_DEFAULT_WINDOW_POSITION_ENABLED);
    assert(result.config.x11.windowPositionX
        == X11_CONFIG_DEFAULT_WINDOW_POSITION_X);
    assert(result.config.x11.windowPositionY
        == X11_CONFIG_DEFAULT_WINDOW_POSITION_Y);
    assert(result.config.x11.panelEnabled == X11_CONFIG_DEFAULT_PANEL_ENABLED);
    assert(result.config.x11.fontName == X11_CONFIG_DEFAULT_FONT_NAME);
}

static void iniTextSourceProducesX11Patch() {
    DeferredLogBuffer diagnostics;
    IniTextConfigSource source("memory",
        "cthugha.no-mit-shm: yes\n"
        "cthugha.root: yes\n"
        "cthugha.install: yes\n"
        "cthugha.no-decorate: yes\n"
        "cthugha.full-screen: yes\n"
        "cthugha.position: -4+9\n"
        "cthugha.panel: yes\n"
        "cthugha.text-on-term: yes\n"
        "cthugha.font: 9x15\n");
    ConfigPatch patch = source.acquire(diagnostics);

    assert(diagnostics.diagnostics().empty());
    assert(*patchValue(patch, "x11.mit_shm") == "0");
    assert(*patchValue(patch, "x11.root_window") == "1");
    assert(*patchValue(patch, "x11.private_cmap") == "1");
    assert(*patchValue(patch, "x11.override_redirect") == "1");
    assert(*patchValue(patch, "x11.fullscreen") == "1");
    assert(*patchValue(patch, "x11.window_position_enabled") == "1");
    assert(*patchValue(patch, "x11.window_position_x") == "-4");
    assert(*patchValue(patch, "x11.window_position_y") == "9");
    assert(*patchValue(patch, "x11.panel_enabled") == "1");
    assert(*patchValue(patch, "x11.text_on_term") == "1");
    assert(*patchValue(patch, "x11.font_name") == "9x15");
}

static void commandLineSourceBuildsX11Config() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--no-mit-shm",
            "--root",
            "--install",
            "--no-decorate",
            "--full-screen",
            "--position",
            "+10+30",
            "--panel",
            "--text-on-term",
            "--font",
            "fixed",
        })
        .build();

    assert(result.ok());
    assert(result.config.x11.mitShm == 0);
    assert(result.config.x11.rootWindow == 1);
    assert(result.config.x11.privateCmap == 1);
    assert(result.config.x11.overrideRedirect == 1);
    assert(result.config.x11.fullscreen == 1);
    assert(result.config.x11.windowPositionEnabled == 1);
    assert(result.config.x11.windowPositionX == 10);
    assert(result.config.x11.windowPositionY == 30);
    assert(result.config.x11.panelEnabled == 1);
    assert(result.config.x11.textOnTerm == 1);
    assert(result.config.x11.fontName == "fixed");
}

int main() {
    fflush(stderr);
    assert(configArgumentsFromArgv(0, NULL).empty());
    defaultsProduceX11ConfigOnlyForX11Builds();
    iniTextSourceProducesX11Patch();
    commandLineSourceBuildsX11Config();
    return 0;
}
