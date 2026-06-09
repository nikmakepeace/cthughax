#include "cthugha.h"
#include "Option.h"
#include "display.h"
#include "CthughaDisplay.h"

OptionDummy optionDummy;

Option::~Option() { }

void OptionOnOff::change(const char* to) {

    if (!strncasecmp("yes", to, 3))
        value = 1;
    else if (!strncasecmp("on", to, 2))
        value = 1;
    else if (!strncasecmp("1", to, 1))
        value = 1;
    else if (!strncasecmp("no", to, 2))
        value = 0;
    else if (!strncasecmp("off", to, 3))
        value = 0;
    else if (!strncasecmp("0", to, 1))
        value = 0;
    else {
        CTH_ERROR("Illegal yes/no-value `%s'.\n", to);
    }
}
