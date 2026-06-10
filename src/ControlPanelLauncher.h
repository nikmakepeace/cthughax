/** @file
 * Port used by key actions to launch or focus the external control panel.
 */

#ifndef CTHUGHA_CONTROL_PANEL_LAUNCHER_H
#define CTHUGHA_CONTROL_PANEL_LAUNCHER_H

class ControlPanelLauncher {
public:
    virtual ~ControlPanelLauncher() { }

    /** Requests launch or focus of the control panel for this app instance. */
    virtual void launchControlPanel() = 0;
};

#endif
