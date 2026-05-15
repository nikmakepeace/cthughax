#include "cthugha.h"
#include "Option.h"
#include "imath.h"

//
// Integer Options
//
void OptionInt::change(const char* to) { setValue(atoi(to)); }
void OptionInt::change(int by) {
    if (maxValue > 0) {
        value = mod(value + by - minValue, maxValue - minValue) + minValue;
    } else {
        value += by;
        if (value < minValue)
            value = minValue;
        if ((maxValue > 0) && (value >= maxValue))
            value = maxValue - 1;
    }
}

void OptionInt::setValue(int value_) {
    if (value_ < minValue) {
        CTH_ERROR("Value %d for option %s too small. Using %d.\n", value_, name(), minValue);
        value_ = minValue;
    }
    if (maxValue && (value_ >= maxValue)) {
        CTH_ERROR("Value %d for option %s too large. Using %d.\n", value_, name(), maxValue - 1);
        value_ = maxValue - 1;
    }
    value = value_;
}

//
// change to String
// str can be: an integer: time in 1/100 of a sec
//             a double with "sec" afterwards: time in seconds
//
void OptionTime::change(const char* str) {

    if (strstr(str, "sec") != NULL) {
        double d;
        if (sscanf(str, "%lfsec", &d) == 0) {
            CTH_ERROR("Not a time value `%s' for option `%s'.\n", str, name());
            return;
        }
        value = int(d * 100 + 0.5);
    } else {
        char* pos;
        int tvalue = strtol(str, &pos, 0);
        if (str == pos) { // not a number
            CTH_ERROR("Not a time value `%s' for option `%s'.\n", str, name());
            return;
        }
        value = tvalue;
    }
    if (value < 0) {
        CTH_ERROR("Time value for `%s' can not be negative. Setting to 0.\n", name());
        value = 0;
    }
}
