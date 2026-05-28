/*
 * Based on README.aztcd from the linux-sources (Tiny Audio CD Player)
 *       and CDPlay 0.31 from Sariel Har-Peled
 * There is not much left from the original code.
 */

#include "cthugha.h"
#include "CDPlayer.h"
#include "Interface.h"

#if WITH_CDROM == 1

#include "keys.h"
#include "display.h"
#include "imath.h"
#include "DisplayDevice.h"

#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_LINUX_CDROM_H
#include <linux/cdrom.h>
#else
#include <sys/cdrom.h>
#endif

char dev_cd[PATH_MAX] = DEV_CDROM;

OptionInt cd_first_track("cd-first-track", 0); // start with track nr
OptionOnOff cd_loop("cd-loop", 0); // restart when all tracks played
OptionOnOff cd_randomplay("cd-random-play", 0); // play random
OptionOnOff cd_eject_on_end("cd-eject-on-end", 0); // eject CD if stoped
OptionOnOff cd_stop_on_exit("cd-stop-on-exit", 0); // stop playing when leaving program

CDPlayer* cdPlayer = NULL;

CDPlayer::CDPlayer()
    : handle(-1)
    , playStart(0)
    , playStop(0)
    , status(NoStatus)
    , first(0)
    , last(0) {

    // autostart CD
    if (int(cd_first_track) > 0)
        if (int(cd_randomplay)) {
            CTH_INFO("  Starting random play of CD\n");
            play(0);
        } else {
            CTH_INFO("  Starting CD at track: %d.\n", int(cd_first_track));
            play(int(cd_first_track));
        }
}

CDPlayer::~CDPlayer() {
    if (handle == -1) // CD not in use
        return;

    if (int(cd_stop_on_exit))
        stop();
}

//
// do regular checks for CD
//
void CDPlayer::operator()() {

    if (handle == -1) // if CD is not in use yet, do nothing here
        return;

    getInfo();

    /* Test whether CD-ROM has completed operation, or if the desired
       track has been reached. Not all CD-ROMs give the CDROM_AUDIO_COMPL.
       status (like mine)
    */
    if ((status == Completed) || (track < playStart) || (track > playStop)) {

        if (int(cd_randomplay))
            next(0);
        else if (int(cd_loop))
            play(1);
        else if (int(cd_eject_on_end) && (status != Ejected))
            eject();
        else
            stop();
    }
}

int CDPlayer::openCD() {
    if (handle != -1) // already open
        return 0;

    CTH_TRACE("Opening CD device `%s'.\n", "cd player", dev_cd);

    // open device
    if ((handle = open(dev_cd, O_RDONLY)) == -1) {
        CTH_ERRNO(errno, "can not open `%s'.", dev_cd);
        return 1;
    }

    // read table of contents
    if (readTOC()) {
        close(handle);
        handle = -1;
        return 1;
    }

    return 0;
}

int CDPlayer::readTOC() {
    if (openCD())
        return 1;

    struct cdrom_tochdr tocHdr;
    struct cdrom_tocentry entry;
    struct cdrom_msf msf;

    if (ioctl(handle, CDROMREADTOCHDR, &tocHdr)) {
        CTH_ERRNO(errno, "Can't read Table of Contents");
        return 1;
    }

    // check, for correct TOC
    first = tocHdr.cdth_trk0;
    last = tocHdr.cdth_trk1;
    if ((first == 0) || (first > last)) {
        CTH_ERROR("Illegal Table of Contents (first track: %d, last track: %d)\n", first, last);
        return 1;
    }

    /* set end-time in msf for fast() */
    entry.cdte_track = CDROM_LEADOUT;
    entry.cdte_format = CDROM_MSF;
    if (ioctl(handle, CDROMREADTOCENTRY, &entry)) {
        CTH_ERRNO(errno, "Can't read TOC entry");
        return 1;
    }
    msf.cdmsf_min1 = entry.cdte_addr.msf.minute;
    msf.cdmsf_sec1 = entry.cdte_addr.msf.second;
    msf.cdmsf_frame1 = entry.cdte_addr.msf.frame;

    // build play list
    nextList = new int[last + 1];
    if (int(cd_randomplay)) {
        int p0 = rand() % (last - first + 1) + first;
        int p = p0;
        nextList[0] = p0;
        for (int i = 1; i < last; i++) {
            nextList[i] = 0;
        }
        for (int i = 0; i < last; i++) {
            int p1;

            // find next
            do {
                p1 = rand() % (last - first + 1) + first;
            } while (nextList[p1] != 0);

            nextList[p] = p1;
            p = p1;
        }
        nextList[p] = int(cd_loop) ? p0 : last + 99;
    } else {
        for (int i = 0; i < first; i++)
            nextList[i] = first;
        for (int i = first; i < last; i++) {
            nextList[i] = i + 1;
        }
        nextList[last] = int(cd_loop) ? first : last + 99;
    }

    char traceLine[512];
    int traceLen = snprintf(traceLine, sizeof(traceLine), "nextList:");
    for (int i = 0; (i < last) && (traceLen < int(sizeof(traceLine))); i++)
        traceLen += snprintf(traceLine + traceLen, sizeof(traceLine) - traceLen,
            " %d", nextList[i]);
    CTH_TRACE("%s\n", "cd player", traceLine);

    return 0;
}

void CDPlayer::getInfo() {

    //    status = NoStatus;		// we know nothing
    track = 0;

    if (openCD())
        return;

    struct cdrom_subchnl subchnl;

    // get CDROM status
    subchnl.cdsc_format = CDROM_MSF;
    if (ioctl(handle, CDROMSUBCHNL, &subchnl)) {
        CTH_ERRNO(errno, "Can't get subchanel status from CD.");
        return;
    }

    switch (subchnl.cdsc_audiostatus) {
    case CDROM_AUDIO_PLAY:
        status = Playing;
        track = subchnl.cdsc_trk;
        absMin = subchnl.cdsc_absaddr.msf.minute;
        absSec = subchnl.cdsc_absaddr.msf.second;
        absFrame = subchnl.cdsc_absaddr.msf.frame;
        relMin = subchnl.cdsc_reladdr.msf.minute;
        relSec = subchnl.cdsc_reladdr.msf.second;
        relFrame = subchnl.cdsc_reladdr.msf.frame;
        break;
    case CDROM_AUDIO_PAUSED:
        status = Paused;
        track = subchnl.cdsc_trk;
        absMin = subchnl.cdsc_absaddr.msf.minute;
        absSec = subchnl.cdsc_absaddr.msf.second;
        absFrame = subchnl.cdsc_absaddr.msf.frame;
        relMin = subchnl.cdsc_reladdr.msf.minute;
        relSec = subchnl.cdsc_reladdr.msf.second;
        relFrame = subchnl.cdsc_reladdr.msf.frame;
        break;
    case CDROM_AUDIO_COMPLETED:
        status = Stopped;
        break;
    }
}

int CDPlayer::eject() {

    delete nextList;
    nextList = NULL;

    if (status == Ejected) {
        if (openCD())
            return 1;
    } else {
        if (openCD())
            return 1;
        if (ioctl(handle, CDROMEJECT)) {
            CTH_ERRNO(errno, "Can not eject CD.");
            return 1;
        }
        status = Ejected;
        delete nextList;
        nextList = NULL;
        close(handle);
        handle = -1;
    }
    return 0;
}

/* pause CD */
int CDPlayer::pause() {

    if (openCD())
        return 1;

    getInfo();

    switch (status) {
    case Paused:
        if (ioctl(handle, CDROMRESUME))
            return 1;
        break;
    default:
        if (ioctl(handle, CDROMPAUSE))
            return 1;
    }

    return 0;
}

/* stop CD */
int CDPlayer::stop() {

    if (openCD())
        return 1;

    if (ioctl(handle, CDROMSTOP)) {
        CTH_ERRNO(errno, "Can not stop CD.");
        return 1;
    }

    delete nextList;
    nextList = NULL;
    close(handle);
    handle = -1;

    status = Stopped;

    return 0;
}

//
// start playing at track
//
int CDPlayer::play(int track) {

    if (openCD())
        return 1;

    struct cdrom_tocentry entry;
    struct cdrom_msf msf;

    //    stop();

    if (track < first)
        track = first;
    if (track > last) {
        track = last;
    }

    /* set range of playing */
    playStart = track;
    playStop = track;
    while (nextList[playStop] == (playStop + 1))
        playStop = nextList[playStop];

    // get the start position of the track
    entry.cdte_track = playStart;
    entry.cdte_format = CDROM_MSF;
    if (ioctl(handle, CDROMREADTOCENTRY, &entry)) {
        CTH_ERRNO(errno, "Can't read TOC Entry for track %d", track);
        return 1;
    }

    // set start position
    msf.cdmsf_min0 = entry.cdte_addr.msf.minute;
    msf.cdmsf_sec0 = entry.cdte_addr.msf.second;
    msf.cdmsf_frame0 = entry.cdte_addr.msf.frame;

    // set end position
    entry.cdte_track = ((playStop + 1) > last) ? CDROM_LEADOUT : playStop + 1;
    entry.cdte_format = CDROM_MSF;
    if (ioctl(handle, CDROMREADTOCENTRY, &entry)) {
        CTH_ERRNO(errno, "Can't read TOC Entry");
        return 1;
    }
    msf.cdmsf_min1 = entry.cdte_addr.msf.minute;
    msf.cdmsf_sec1 = entry.cdte_addr.msf.second;
    msf.cdmsf_frame1 = entry.cdte_addr.msf.frame;

    if (ioctl(handle, CDROMSTART)) {
        CTH_ERRNO(errno, "Can't start CD");
        return 1;
    }
    if (ioctl(handle, CDROMPLAYMSF, &msf)) {
        CTH_ERRNO(errno, "Can't play CD.");
        return 1;
    }

    return 0;
}

/* go to next track */
int CDPlayer::next(int skip) {
    if (openCD())
        return 1;

    getInfo();

    int t = track;
    if (skip > 0)
        t = nextList[t];
    else
        t = track - 1;
    return play(t);
}

int CDPlayer::fast(int skip) {
    struct cdrom_tocentry entry;
    struct cdrom_msf msf;

    if (openCD())
        return 1;

    getInfo();

    if (status != Playing)
        return 0;

    /* calculate the current position in seconds and add skip */
    int pos = absMin * CD_SECS + absSec + skip;

    if (pos < 0) // skip before begin of CD
        return play(0); // start playing at firs track

    // set new start position
    msf.cdmsf_min0 = pos / CD_SECS;
    msf.cdmsf_sec0 = pos % CD_SECS;
    msf.cdmsf_frame0 = absFrame;

    entry.cdte_track = ((playStop + 1) > last) ? CDROM_LEADOUT : playStop + 1;
    entry.cdte_format = CDROM_MSF;
    if (ioctl(handle, CDROMREADTOCENTRY, &entry)) {
        CTH_ERRNO(errno, "Can't read TOC Entry");
        return 1;
    }
    msf.cdmsf_min1 = entry.cdte_addr.msf.minute;
    msf.cdmsf_sec1 = entry.cdte_addr.msf.second;
    msf.cdmsf_frame1 = entry.cdte_addr.msf.frame;

    /* Play it */
    if (ioctl(handle, CDROMSTART)) {
        CTH_ERRNO(errno, "Can't start CD");
        return 1;
    }
    if (ioctl(handle, CDROMPLAYMSF, &msf)) {
        CTH_ERRNO(errno, "Drive error or invalid address\n");
    }

    return 0;
}

//
// the user interface
//
class InterfaceCD : public Interface {
    char cdText[512];

public:
    InterfaceCD()
        : Interface("CD", "CD Player", NULL) {

        nElements = 5;
        elements = new InterfaceElement*[nElements];
        elements[0] = new InterfaceElementOption("Autostart track: %10s", &cd_first_track);
        elements[1] = new InterfaceElementOption("Loop CD        : %10s", &cd_loop);
        elements[2] = new InterfaceElementOption("Random Play    : %10s", &cd_randomplay);
        elements[3] = new InterfaceElementOption("Eject on end   : %10s", &cd_eject_on_end);
        elements[4] = new InterfaceElementOption("Stop at exit   : %10s", &cd_stop_on_exit);
    }

    void preRun() {
        static char str[128];

        switch (cdPlayer->status) {
        case CDPlayer::Playing:
            sprintf(str, "CD-Player Track: %d %d:%02d (%d - %d) %d:%02d", cdPlayer->track,
                cdPlayer->relMin, cdPlayer->relSec, cdPlayer->first, cdPlayer->last,
                cdPlayer->absMin, cdPlayer->absSec);
            break;
        case CDPlayer::Paused:
            sprintf(str, "CD-Player Track: %d %d:%02d (%d - %d) %d:%02d pause", cdPlayer->track,
                cdPlayer->relMin, cdPlayer->relSec, cdPlayer->first, cdPlayer->last,
                cdPlayer->absMin, cdPlayer->absSec);
            break;
        case CDPlayer::Ejected:
            sprintf(str, "CD-Player: ejected");
            break;
        default:
            sprintf(str, "CD-Player:");
        }

        sprintf(cdText,
            "<- prev(7)  pause(8)  -> next(9)\n"
            "<< rev(4)   stop(5)   >> fwd(6) \n"
            "           close/eject(2)       \n"
            "%s",
            str);
        text = cdText;
    }
} interfaceCD;

ACTION(CDnext) {
    if (cdPlayer)
        cdPlayer->next(int(v));
}
ACTION(CDfast) {
    if (cdPlayer)
        cdPlayer->fast(int(v));
}
ACTION(CDpause) {
    if (cdPlayer)
        cdPlayer->pause();
}
ACTION(CDstop) {
    if (cdPlayer)
        cdPlayer->stop();
}
ACTION(CDcloseEject) {
    if (cdPlayer)
        cdPlayer->eject();
}

#else

//
// CDROM disabled
//

char dev_cd[PATH_MAX] = "";

CDPlayer* cdPlayer = NULL;

#endif
