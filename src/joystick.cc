#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include "joystick.h"
#include "CoreOption.h"
#include "CthughaDisplay.h"

int Joystick::useJoystick = 0;

int Joystick::js = -1;
int Joystick::init = 1;
int Joystick::axis[4] = { 0, 0, 0, 0 };
int Joystick::stiffAxis[4] = { 8000, 9000, -5000, 0 };

int Joystick::active = 0;

void Joystick::run() {
#ifdef HAVE_JOYSTICK
    if (init) {
        init = 0;

        if (!useJoystick)
            return;

        if ((js = open("/dev/js0", O_RDONLY | O_NONBLOCK)) < 0) {
            CTH_ERROR("Can not open `/dev/js0'.\n");
            return;
        }

        char n;
        ioctl(js, JSIOCGAXES, &n);
        CTH_DEBUG("   joystick: %d axes.\n", n);
        ioctl(js, JSIOCGBUTTONS, &n);
        CTH_DEBUG("   joystick: %d buttons.\n", n);

        // TODO: Test if joystick driver is new enough.

        active = 1;
    }

    if (js > 0) {
        js_event e;
        while (read(js, &e, sizeof(js_event)) > 0) {
            if (e.type & JS_EVENT_BUTTON) {
                switch (e.number) {
                case 0:
                    if (e.value)
                        active = 1 - active;
                    break;
                case 1:
                    if (e.value)
                        CoreOption::changeAll();
                    break;
                case 2:
                    if (e.value)
                        CoreOption::changeOne();
                    break;
                }
            } else if ((e.type & JS_EVENT_AXIS) && (e.number < 4)) {
                axis[e.number] = e.value;
            }
        }
        if (active) { // set new position if button is pressed
            stiffAxis[0] = axis[0];
            stiffAxis[1] = axis[1];
        }
    } else {
        axis[0] = stiffAxis[0] = int(20000.0 * sin(0.2 * now));
        axis[1] = stiffAxis[1] = int(20000.0 * sin(0.5 * now));
    }
#endif
}
