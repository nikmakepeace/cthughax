/** @file
 * No-op runtime control-panel service.
 */

#include "ControlPanelService.h"

class NullControlPanelService : public ControlPanelService {
public:
    virtual void prepare() { }
    virtual void toggle() { }
    virtual void hide() { }
    virtual void pumpEvents() { }
    virtual void syncFromRuntime() { }
};

std::unique_ptr<ControlPanelService> createNullControlPanelService() {
    return std::unique_ptr<ControlPanelService>(new NullControlPanelService());
}
