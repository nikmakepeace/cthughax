/** @file
 * wxWidgets implementation of the optional runtime control panel.
 */

#include "CthughaPanel.h"

#include "ControlPanelService.h"
#include "ProcessServices.h"

#include <wx/app.h>
#include <wx/evtloop.h>

#if !defined(_WIN32)
#include <signal.h>
#include <string.h>
#endif

#include <vector>

class CthughaControlPanelWxApp : public wxApp {
public:
    virtual bool OnInit() {
        SetExitOnFrameDelete(false);
        return true;
    }
};

wxIMPLEMENT_APP_NO_MAIN(CthughaControlPanelWxApp);

namespace {

#if !defined(_WIN32)
class ScopedSignalActionPreserver {
    int signalNumber;
    int valid;
    struct sigaction savedAction;

public:
    explicit ScopedSignalActionPreserver(int signalNumber_)
        : signalNumber(signalNumber_)
        , valid(0) {
        memset(&savedAction, 0, sizeof(savedAction));
        valid = sigaction(signalNumber, 0, &savedAction) == 0;
    }

    ~ScopedSignalActionPreserver() {
        if (valid)
            sigaction(signalNumber, &savedAction, 0);
    }
};
#endif

}

class WxControlPanelService : public ControlPanelService {
    ControlPanelRuntimePorts ports;
    LogSink& log;
    CthughaPanel* panel;
    int started;
    int disabled;
    int entryArgc;
    std::vector<char*> entryArgv;

    int ensureStarted() {
        if (disabled)
            return 0;
        if (started)
            return 1;

#if defined(__APPLE__)
        log.warn("The in-process wx control panel is disabled on macOS; "
                 "wxWidgets host startup does not return reliably inside "
                 "the SDL3 visualizer process.\n");
        disabled = 1;
        return 0;
#else
        log.debug("wx control panel: starting wx host.\n");
        entryArgc = ports.processArgc;
        entryArgv.clear();
        if (entryArgc > 0 && ports.processArgv != 0) {
            entryArgv.assign(ports.processArgv,
                ports.processArgv + entryArgc);
            entryArgv.push_back(0);
        } else {
            static char fallbackAppName[] = "cthugha";
            entryArgv.push_back(fallbackAppName);
            entryArgv.push_back(0);
            entryArgc = 1;
        }
#if !defined(_WIN32)
        ScopedSignalActionPreserver preserveSigint(SIGINT);
        ScopedSignalActionPreserver preserveSigterm(SIGTERM);
#endif
        if (!wxEntryStart(entryArgc, &entryArgv[0])) {
            log.error("Can not initialize wxWidgets control panel.\n");
            disabled = 1;
            return 0;
        }
        started = 1;

        if (wxTheApp == 0 || !wxTheApp->CallOnInit()) {
            log.error("Can not start wxWidgets control panel app.\n");
            wxEntryCleanup();
            started = 0;
            disabled = 1;
            return 0;
        }

        log.debug("wx control panel: wx host started.\n");
        return 1;
#endif
    }

    void ensurePanel() {
        if (panel == 0) {
            log.debug("wx control panel: creating hidden panel frame.\n");
            panel = new CthughaPanel(ports);
            panel->Hide();
            log.debug("wx control panel: hidden panel frame ready.\n");
        }
    }

public:
    WxControlPanelService(const ControlPanelRuntimePorts& ports_, LogSink& log_)
        : ports(ports_)
        , log(log_)
        , panel(0)
        , started(0)
        , disabled(0)
        , entryArgc(0)
        , entryArgv() { }

    virtual ~WxControlPanelService() {
        if (panel != 0) {
            panel->Hide();
            panel->Destroy();
            panel = 0;
            pumpEvents();
        }

        if (started) {
            if (wxTheApp != 0)
                wxTheApp->OnExit();
            wxEntryCleanup();
            started = 0;
        }
    }

    virtual void prepare() {
        if (!ensureStarted())
            return;

        ensurePanel();
        pumpEvents();
    }

    virtual void toggle() {
        if (!ensureStarted())
            return;

        ensurePanel();
        if (panel->IsShown()) {
            panel->Hide();
        } else {
            panel->syncFromRuntime();
            panel->Show();
            panel->Raise();
        }
    }

    virtual void hide() {
        if (panel != 0)
            panel->Hide();
    }

    virtual void pumpEvents() {
        if (!started || wxTheApp == 0)
            return;

        wxEventLoopBase* eventLoop = wxEventLoopBase::GetActive();
        if (eventLoop != 0) {
            for (int dispatched = 0; dispatched < 32; dispatched++) {
                int result = eventLoop->DispatchTimeout(0);
                if (result <= 0)
                    break;
            }
            eventLoop->ProcessIdle();
        }

        wxTheApp->ProcessPendingEvents();
        wxTheApp->ProcessIdle();
    }

    virtual void syncFromRuntime() {
        if (panel != 0 && panel->IsShown())
            panel->syncFromRuntime();
    }
};

std::unique_ptr<ControlPanelService> createWxControlPanelService(
    const ControlPanelRuntimePorts& ports, LogSink& log) {
    return std::unique_ptr<ControlPanelService>(
        new WxControlPanelService(ports, log));
}
