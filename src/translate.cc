#include "cthugha.h"
#include "translate.h"
#include "display.h"
#include "Interface.h"
#include "tab_header.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

OptionOnOff use_translates("use-translate", 1); /* allow translations */
OptionOnOff trans_stretch("stretching", 1); /* allow stretching */

OptionOnOff transLoadOnDemand("load-on-demand", 1);
OptionOnOff transLoadLate("load-late", 1);

static CoreOptionEntry* _trans[] = { new TranslateEntry("none", "No Translate") };
static CoreOptionEntryList translateEntries(_trans, 1);

char lib_size[512];
static const char* translate_path[] = { "./", "./tab/", lib_size, CTH_LIBDIR "/tab/", "" };

/*
 * Initialize the translate-tables.
 */
int init_translate() {

    if (use_translates) {

        CTH_INFO("  loading translation tables...\n");

        if (transLoadLate)
            transLoadOnDemand.setValue(1);

        sprintf(lib_size, CTH_LIBDIR "/tab/%dx%d/", BUFF_WIDTH, BUFF_HEIGHT);

        CthughaBuffer::current->translate.load(
            translate_path, "/tab/", ".cmd", TranslateEntry::loaderCmd);
        CthughaBuffer::current->translate.load(
            translate_path, "/tab/", ".tab", TranslateEntry::loaderTab);

        CTH_INFO("  number of loaded translates: %d\n",
            CthughaBuffer::current->translate.getNEntries());
    }

    return 0;
}

//
// Read a translate file
//
static int* read_trans_data(FILE* file, const tab_header& header, int BSize, const char* name) {
    union data {
        unsigned long* l;
        unsigned short* s;
    } D;
    int i, j;

    int size = (BSize > 65535) ? sizeof(long) : sizeof(short);

    int* trans = new int[BSize];
    int* dst = trans;

    /* allocate memory for read buffer */
    D.l = new unsigned long[header.size_x];
    if ((void*)D.l != (void*)D.s) {
        CTH_ERROR("Wackiness afoot at %d in %s\n", __LINE__, __FILE__);
        exit(1);
    }

    /* read data */
    for (i = 0; i < header.size_y; i++) {
        if ((j = fread(D.l, size, header.size_x, file)) < header.size_x) {
            CTH_ERROR("  Can't read at line: %d, read: %d (%s)\n", i, j, name);
            delete[] D.l;
            delete[] trans;
            return NULL;
        }
        for (j = 0; j < header.size_x; j++) {
            if (BSize > 65535) {
                if (D.l[j] >= (unsigned int)(BSize)) {
                    CTH_ERROR("  High-translate (value: %ld) in %s.\n", D.l[j], name);
                    delete[] D.l;
                    delete[] trans;
                    return NULL;
                }
                *dst++ = (int)D.l[j];
            } else {
                if (D.s[j] >= (unsigned int)(BSize)) {
                    CTH_ERROR("  High-translate (value: %d) in %s.\n", D.s[j], name);
                    delete[] D.l;
                    delete[] trans;
                    return NULL;
                }
                *dst++ = (int)D.s[j];
            }
        }
    }

    delete[] D.l;

    /* Check for too much data */
    if (fread(&D, size, 1, file)) {
        CTH_ERROR("  Extra data at end of file %s\n", name);
        delete[] trans;
        return NULL;
    }

    return trans;
}

static int* stretch_trans(const int* src, const tab_header& header) {
    double xs, ys;
    int x, y, tp, ox, oy, dx, dy;
    int i, j;

    CTH_DEBUG(" ... stretching");

    int* dst = new int[BUFF_HEIGHT * BUFF_WIDTH];

    xs = (double)header.size_x / BUFF_WIDTH;
    ys = (double)header.size_y / BUFF_HEIGHT;

    for (j = 0; j < BUFF_HEIGHT; j++) {

        y = int((double)j * ys);
        if (y >= header.size_y)
            y = header.size_y - 1;

        for (i = 0; i < BUFF_WIDTH; i++) {

            x = int((double)i * xs);
            if (x >= header.size_x)
                x = header.size_x - 1;

            tp = src[x + y * header.size_x];
            ox = tp % header.size_x;
            oy = tp / header.size_x;
            dx = int((double)(ox - x) / xs);
            dy = int((double)(oy - y) / ys);
            dst[i + j * BUFF_WIDTH] = abs(i + dx + (j + dy) * BUFF_WIDTH) % BUFF_SIZE;
        }
    }
    return dst;
}

CoreOptionEntry* TranslateEntry::loaderTab(
    FILE* file, const char* name, const char* dir, const char* total_name) {
    tab_header header;

    int BSize = BUFF_SIZE;
    int stretch = 0;
    TranslateEntry* new_trans;

    /* read header */
    if (fread(&header, sizeof(tab_header), 1, file) != 1) {
        CTH_ERROR("  Can't read header from file '%s'.\n", name);
        return NULL;
    }

    /* check header ID */
    if (header.id != *((long*)"HDKB")) {
        CTH_WARN("\n  Header ID-mismatch. Trying without header.");

        rewind(file); // back to start of file

        /* fill in header */
        header.size_x = BUFF_WIDTH;
        header.size_y = BUFF_HEIGHT;

        new_trans = new TranslateEntry(name, "");
    } else {
        /* ID OK - now test size */
        if ((header.size_x != BUFF_WIDTH) || (header.size_y != BUFF_HEIGHT)) {
            CTH_WARN("\n    Size mismatch (%dx%d instead of %dx%d)", header.size_x, header.size_y,
                BUFF_WIDTH, BUFF_HEIGHT);
            if (int(trans_stretch)) { /* allow stretching */
                stretch = 1;
            } else {
                return NULL;
            }
        }

        /* translate table might use a different size */
        BSize = header.size_x * header.size_y;

        // make sure there is no \n in the name
        char* ent = strchr(header.description, '\n');
        if (ent != NULL)
            *ent = '\0';

        new_trans = new TranslateEntry(name, header.description);
    }

    if (transLoadOnDemand) {
        if (TranslateEntry::cmdRead[0] == '\0') {
            CTH_ERROR("Could not find 'cmdRead'. Can not load .tab files on demand.\n");
            transLoadOnDemand.setValue(0);
        } else {
            sprintf(new_trans->command, TranslateEntry::cmdRead, total_name);
            new_trans->trans = NULL;
            return new_trans;
        }
    }

    /* read data */
    int* trans = read_trans_data(file, header, BSize, name);
    if (trans == NULL) {
        delete new_trans;
        return NULL;
    }

    /* do the stretching if necessary (from: Rus Maxham) */
    if (stretch) {
        new_trans->trans = stretch_trans(trans, header);
        delete[] trans;
    } else
        new_trans->trans = trans;

    return new_trans;
}

/*
 * read a file in the form:
 *  cmdtab
 *  <descrption>
 *  <commd> [args] %d %d
 */
#define MAX_DESC_LEN 64
CoreOptionEntry* TranslateEntry::loaderCmd(
    FILE* file, const char* name, const char* dir, const char* total_name) {
    char line[PATH_MAX];
    char command[PATH_MAX];
    FILE* cmd_file;
    TranslateEntry* new_trans;

    /* check ID */
    fgets(line, 512, file);
    if (strncmp(line, "cmdtab", 6) != 0) {
        CTH_ERROR("  Not a command translate file: %s.\n", name);
        return NULL;
    }

    // get description
    fgets(line, MAX_DESC_LEN, file);
    line[MAX_DESC_LEN - 1] = '\0'; // make sure the string terminates

    if (strchr(line, '\n') != NULL) // delete trailing \n
        *(strchr(line, '\n')) = '\0';

    new_trans = new TranslateEntry(name, line);

    /* get command */
    fgets(line, PATH_MAX, file);

    /* generate the command line */
    char cmd2[PATH_MAX];
    strncpy(cmd2, dir, PATH_MAX);
    strncat(cmd2, "/", PATH_MAX);
    strncat(cmd2, line, PATH_MAX);
    sprintf(command, cmd2, BUFF_WIDTH, BUFF_HEIGHT);

    if (strchr(command, '\n') != NULL) { // delete trailing '\n`
        *(strchr(command, '\n')) = '\0';
    }

    //
    // check for the helping program cmdRead
    //
    if (strcmp(name, "cmdRead") == 0) {
        strncpy(TranslateEntry::cmdRead, command, PATH_MAX);
        delete new_trans;
        CTH_DEBUG(" (auxiliary program)\n");
        return NULL;
    }

    if (transLoadOnDemand) {
        strncpy(new_trans->command, command, PATH_MAX);
        new_trans->trans = NULL;
    } else {
        /* start the command */
        CTH_DEBUG("\n    starting: %s", command);
        if ((cmd_file = popen(command, "r")) == NULL) {
            CTH_ERRNO(errno, "  Can't run command '%s'.\n", command);
            return NULL;
        }

        new_trans->trans = new int[BUFF_SIZE];

        for (int i = 0; i < BUFF_HEIGHT; i++) {
            if (new_trans->loadLine(cmd_file, i)) {
                delete new_trans;
                return NULL;
            }
        }

        pclose(cmd_file);
    }

    return new_trans;
}

char TranslateEntry::cmdRead[PATH_MAX] = "";

int TranslateEntry::loadLine(FILE* in, int n) {

    // load line into temporary buffer
    long line[BUFF_WIDTH];
    if (fread(line, sizeof(long), BUFF_WIDTH, in) != (size_t)BUFF_WIDTH) {
        CTH_ERRNO(errno, "  reading translation table %s. failed at line %d.", name, n);
        return 1;
    }

    // check for errors in the line,
    // copy into the real buffer
    int* d = trans + BUFF_WIDTH * n;
    for (int i = 0; i < BUFF_WIDTH; i++, d++) {
        if ((line[i] >= BUFF_SIZE) || (line[i] < 0)) {
            CTH_ERROR("  illegal value in translation table %s.\n", name);
            return 1;
        }
        *d = line[i];
    }

    return 0;
}

//
// Do the translate
//
int TranslateEntry::operator()() {
    int i;
    unsigned int* dst;
    unsigned char* src;

    if (trans == NULL)
        return 0;

    int* trans = this->trans;

    if (CthughaBuffer::current->done_translate)
        return 0;

    dst = (unsigned int*)passive_buffer;
    src = active_buffer;
    active_buffer = passive_buffer;
    passive_buffer = src;

    src[0] = 0;

    // thanks to Antonio Schifano (schifano@cli.di.unipi.it)
    // for finding the endianess bug here

#if (__BYTE_ORDER == __BIG_ENDIAN)
    /* always write 4 values at once */
    for (i = BUFF_SIZE / 4; i != 0; i--) { /* do the translation */
        unsigned int D = src[*trans++];
        D <<= 8;
        D += src[*trans++];
        D <<= 8;
        D += src[*trans++];
        D <<= 8;
        *dst++ = D + src[*trans++];
    }
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
    for (i = BUFF_SIZE / 4; i != 0; i--) { /* do the translation */
        unsigned int D = src[trans[0]];
        D += src[trans[1]] << 8;
        D += src[trans[2]] << 16;
        *dst = D + (src[trans[3]] << 24);
        trans += 4;
        dst++;
    }
// #elif
// #error unknown endianess
#endif

    return 0;
}

TranslateOption::TranslateOption(int buffer, const char* name)
    : CoreOption(buffer, name, translateEntries)
    , lodCommonTrans(NULL)
    , lodPipe(NULL)
    , lodPID(0)
    , lodLine(-1)
    , lodCurrent(-1) { }

int TranslateOption::openPipe(const char* command) {

    int pipeDes[2];
    if (pipe(pipeDes)) {
        CTH_ERRNO(errno, "Can not create pipe.");
        return 1;
    }

    switch (lodPID = fork()) {
    case -1:
        CTH_ERRNO(errno, "Can not fork for translation reading program.");
        return 1;
    case 0:
        close(pipeDes[0]); // close writing side of pipe

        nice(99); // be very nice

        // wait until the parent is ready for reading
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(pipeDes[1], &wfds);
        select(FD_SETSIZE, NULL, &wfds, NULL, NULL);

        dup2(pipeDes[1], fileno(stdout)); // make new stdout

        char* argv[4];
        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = (char*)command;
        argv[3] = 0;
        execv("/bin/sh", argv);

        CTH_ERRNO(errno, "Could not execute translation table program.");
        close(pipeDes[0]);
        abort();

    default:
        close(pipeDes[1]); // close reading side of pipe
        if ((lodPipe = fdopen(pipeDes[0], "r")) == NULL) {
            CTH_ERRNO(errno, "Can not associate FILE to pipe.");
            return 1;
        }
    }
    return 0;
}
int TranslateOption::operator()() {

    if ((value < 0) || (value >= getNEntries()))
        return 0;

    TranslateEntry* current = (TranslateEntry*)(entries[value]);

    //
    // do load on demand
    //

    // initialize the common translation table
    if (lodCommonTrans == NULL) {
        lodCommonTrans = new int[BUFF_SIZE];
        for (int i = 0; i < BUFF_SIZE; i++)
            lodCommonTrans[i] = i;
    }

    // check for currently loading translation table
    if (value != lodCurrent) {
        lodCurrent = value;

        if (lodPipe != NULL) {
            if (lodPID > 0) {
                kill(lodPID, 9);
                int status;
                waitpid(lodPID, &status, 0);
                lodPID = 0;
            }
            pclose(lodPipe);
            lodPipe = NULL;
        }

        // start a new command
        if (current->command[0] != '\0') {
            CTH_DEBUG("    starting: %s\n", current->command);
            if (openPipe(current->command)) {
                CTH_ERRNO(errno, "  Can't run command '%s'.\n", current->command);
                return 1;
            }

            current->trans = lodCommonTrans;
            lodLine = 0;
        }

    } else if (lodPipe != NULL) {

        if (lodLine < BUFF_HEIGHT) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(fileno(lodPipe), &rfds);
            struct timeval timeout;

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            // load at most 10 lines at once, and only as long as there is
            // some input data available
            for (int l = 0; (l < 10) && (lodLine < BUFF_HEIGHT)
                && (select(FD_SETSIZE, &rfds, NULL, NULL, &timeout) > 0);
                l++) {
                if (current->loadLine(lodPipe, lodLine)) {
                    pclose(lodPipe);
                    lodPipe = NULL;
                    lodLine = -1;
                    return 1;
                }

                lodLine++;
            }
        } else {
            pclose(lodPipe);
            lodPipe = NULL;
            CTH_DEBUG("    finished loading.\n");

            if (transLoadLate) {
                // keep a copy of this translation table
                current->trans = new int[BUFF_SIZE];
                memcpy(current->trans, lodCommonTrans, BUFF_SIZE * sizeof(int));
                // do not load again
                current->command[0] = '\0';
            } else {
                current->trans = lodCommonTrans;
            }
        }
    }

    //
    // do the translation
    //
    if (lodPipe != NULL)
        return 0;

    return (entries[value]->operator()());
}

const char* TranslateOption::status() {
    if (lodPipe != NULL)
        return "loading";
    return "";
}
