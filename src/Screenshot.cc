#include "Screenshot.h"

char display_prt_file[PATH_MAX] = "PrintScreen"; /* filename used by PrtScrn */

char* prtFileName(const char* ext) {
    static char name[PATH_MAX];
    static int count = 0;

    if (count == 0) {
        sprintf(name, "%s.%s", display_prt_file, ext);
    } else {
        sprintf(name, "%s.%d.%s", display_prt_file, count, ext);
    }
    count++;

    return name;
}
