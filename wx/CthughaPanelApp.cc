/** @file
 * wx application entry point for the separate Cthugha control panel.
 */

#include "CthughaPanelFrame.h"
#include "ControlTransport.h"

#include <string>

#include <wx/app.h>
#include <wx/string.h>

#ifndef _WIN32
#include <dirent.h>
#endif

namespace {

static std::string wxToUtf8(const wxString& value) {
    return value.ToStdString();
}

static int startsWith(const std::string& value, const char* prefix) {
    std::string needle = prefix;
    return value.compare(0, needle.size(), needle) == 0;
}

static std::string parseEndpoint(int argc, wxChar** argv) {
    for (int i = 1; i < argc; i++) {
        std::string arg = wxToUtf8(wxString(argv[i]));
        if (arg == "--control-endpoint" && i + 1 < argc)
            return wxToUtf8(wxString(argv[i + 1]));
        if (startsWith(arg, "--control-endpoint="))
            return arg.substr(std::string("--control-endpoint=").size());
    }
    return "";
}

static std::string discoverEndpoint() {
#ifdef _WIN32
    return "";
#else
    std::string directory = controlDefaultRuntimeDirectory();
    DIR* dir = opendir(directory.c_str());
    if (dir == 0)
        return "";

    std::string endpoint;
    struct dirent* entry = 0;
    while ((entry = readdir(dir)) != 0) {
        std::string name = entry->d_name;
        if (!startsWith(name, "cthugha-control-"))
            continue;
        if (name.size() < 5
            || name.compare(name.size() - 5, 5, ".sock") != 0)
            continue;
        endpoint = "unix:" + directory + "/" + name;
        break;
    }
    closedir(dir);
    return endpoint;
#endif
}

class CthughaPanelApp : public wxApp {
public:
    virtual bool OnInit() {
        std::string endpoint = parseEndpoint(argc, argv);
        if (endpoint.empty())
            endpoint = discoverEndpoint();

        CthughaPanelFrame* frame = new CthughaPanelFrame(endpoint);
        frame->Show(true);
        return true;
    }
};

}

wxIMPLEMENT_APP(CthughaPanelApp);
