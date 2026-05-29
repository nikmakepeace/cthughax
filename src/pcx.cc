#include "cthugha.h"
#include "display.h"
#include "Interface.h"
#include "disp-sys.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"
#include "CthughaFrameBuffer.h"
#include "CthughaDisplay.h"
#include "VisualDirector.h"
#include "imath.h"

#include <unistd.h>

int display_use_pcx = 1; /* allow pcx-usage */

static CoreOptionEntry* _pcx = new PCXEntry("none", "");

CoreOptionEntryList pcxEntries(_pcx);

int* pcx_palettes = NULL; /* index to corresp. palette */

static const char* pcx_path[] = { "./", "./pcx/", CTH_LIBDIR "/pcx/", "" };
CoreOptionEntry* read_pcx(FILE* file, const char* name, const char* dir, const char* total_name);

unsigned char* tempscrn = NULL;

char display_prt_file[PATH_MAX] = "PrintScreen"; /* filename used by PrtScrn */

//
// generate a new filename for print screen (-> move to different file)
//
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

/*
 * Initialization
 */
int init_pcx() {

    if (display_use_pcx) {

        CTH_INFO("  loading PCX-files...\n");

        tempscrn = new unsigned char[BUFF_SIZE];

        CthughaBuffer::current->pcx.load(pcx_path, "/pcx/", ".pcx", read_pcx);

        delete[] tempscrn;

        CTH_INFO("  number of loaded PCX-files: %d\n", CthughaBuffer::current->pcx.getNEntries());
    }

    return 0;
}

int show_pcx() {
    VisualDirector::requestImageStage();
    return 0;
}

static int choose_image_left(int imageSize, int bufferSize) {
    if (imageSize > bufferSize)
        return -(Random(imageSize - bufferSize + 1));

    return Random(bufferSize - imageSize + 1);
}

/*
 * bring the active_pcx to the frame buffer
 */
int show_pcx(CthughaFrameBuffer& frameBuffer) {

    CTH_DEBUG("showing pcx `%s'...\n", CthughaBuffer::current->pcx.currentName());
    PCXEntry* pcxE = (PCXEntry*)CthughaBuffer::current->pcx.current();

    if ((pcxE == NULL) || (pcxE->data == NULL))
        return 0;

    unsigned char* active = frameBuffer.active();
    unsigned char* passive = frameBuffer.passive();
    if (active == NULL || passive == NULL)
        return 0;

    int width = frameBuffer.width();
    int height = frameBuffer.height();
    int pitch = frameBuffer.pitch();

    int x = choose_image_left(pcxE->width, width);
    int y = choose_image_left(pcxE->height, height);

    int src_x = (x < 0) ? -x : 0;
    int dst_x = (x > 0) ? x : 0;
    int copy_w = min(pcxE->width - src_x, width - dst_x);

    int src_y = (y < 0) ? -y : 0;
    int dst_y = (y > 0) ? y : 0;
    int copy_h = min(pcxE->height - src_y, height - dst_y);

    if (copy_w <= 0 || copy_h <= 0)
        return 0;

    for (int row = 0; row < copy_h; row++) {
        unsigned char* src = pcxE->data + (src_y + row) * pcxE->width + src_x;
        unsigned char* active_dst = active + (dst_y + row) * pitch + dst_x;
        unsigned char* passive_dst = passive + (dst_y + row) * pitch + dst_x;

        memcpy(active_dst, src, copy_w);
        memcpy(passive_dst, src, copy_w);
    }

    return 0;
}

/*****************************************************************************/

/* load 1 pcx-file from disk

   Code for loading the pcx-file is from "showp.c" of the original Cthugha.

   Original by: Free Software by TapirSoft Gisbert W.Selke, Dec 1991
     changed to C-Source by Volker Muehle Jul 94
     changed for CTHUGHA by Volker Muehle Sep 94
     changed for Cthugha-L by Deischinger Harald April/May/June 95
     (now the source is readable :) now works for screens > 64K
     removed bug with too wide PCX-files.
     removed some of the for cthugha useless code
     rewrote, so no seek is necessary
     changed for C++
     use ferror instad of errno in getnextchunk
     removed some for cthugha useless code
*/
typedef unsigned char byte;
typedef unsigned short word;
typedef int boolean;
#define MAXLINLEN 2048 /* maximum length of screen line */
typedef struct {
    byte id; /* must be $0A  */
    byte version; /* 0, 2, 3, or 5  */
    byte compr; /* 1 if RLE-coded  */
    byte bitsperpixel;
    word xmin;
    word ymin;
    word xmax;
    word ymax;
    word horidpi; /* horizontal res., dots per inch */
    word vertdpi; /* vertical   res., dots per inch */
    byte colormap[16][3];
    byte reserved;
    byte ncolplanes; /* number of colour planes; max 4 */
    word bytesperline; /* must be even */
    word greyscale; /* 1 if color or b/w;
                       2 if greyscale */
    byte filler[58];
} headrec;

static headrec header;
static int iread, thisbyte;
static boolean zdecomp, zcompr;
static byte repeatct;

FILE* picf;

/* get next chunk from input file  */
void getnextchunk(void) {
    if (feof(picf))
        iread = 0;
    else {
        iread = fread(tempscrn, 1, BUFF_SIZE, picf);
        if (ferror(picf))
            iread = 0;
    }

    thisbyte = 0;
}

/* reads next byte from input file, handling compression */
byte getnextbyte(void) {
    byte res;
    if (!zdecomp) {
        if (thisbyte >= iread)
            getnextchunk();
        if (thisbyte < iread) {
            thisbyte++;
            if (zcompr && tempscrn[thisbyte - 1] >= 192) {
                repeatct = tempscrn[thisbyte - 1] & 0x3F;
                zdecomp = (repeatct > 0) ? 1 : 0;
                if (thisbyte >= iread)
                    getnextchunk();
                thisbyte++;
            }
        }
    }
    if (zdecomp) {
        res = tempscrn[thisbyte - 1];
        repeatct--;
        zdecomp = (repeatct > 0) ? 1 : 0;
    } else {
        if (iread > 0) {
            res = tempscrn[thisbyte - 1];
        } else
            res = 0;
    }
    return res;
}

CoreOptionEntry* read_pcx(
    FILE* file, const char* name, const char* /* dir */, const char* /*total_name*/) {
    int x, y;
    byte bitsperplane;

    unsigned char* buff; /* target-buffer */
    unsigned char* pal; /* palette for this pcx */

    picf = file;

    /* read header */
    iread = fread(&header, 1, sizeof(header), picf);
    if (iread != sizeof(header)) {
        CTH_ERROR("Can't read head of file: %s\n", name);
        return NULL;
    }

    CTH_TRACE("version:%d, compr:%d, ncolpl.:%2d, greysc.:%d, X:%d-%d, Y:%d-%d\n", "pcx",
        header.version, header.compr, header.ncolplanes, header.greyscale, header.xmin, header.xmax,
        header.ymin, header.ymax);

    /* Check header */
    if (header.id != 0x0A) {
        CTH_ERROR("Illegal PCX header (wrong ID %d) in file: %s\n", header.id, name);
        return NULL;
    }
    if (header.version != 0 && header.version != 2 && header.version != 3 && header.version != 5) {
        CTH_ERROR("Illegal PCX header (wrong version %d) in file: %s\n", header.version, name);
        return NULL;
    }
    if (header.compr != 0 && header.compr != 1) {
        CTH_ERROR("Illegal PCX header (wrong compression %d) in file: %s\n", header.compr, name);
        return NULL;
    }
    if (header.ncolplanes > 4) {
        CTH_ERROR("Illegal PCX header (wrong ncolplanes %d) in file: %s\n", header.ncolplanes, name);
        return NULL;
    }
    if (header.greyscale != 1 && header.greyscale != 2)
        header.greyscale = 1;

    if (header.ncolplanes == 0)
        header.ncolplanes = 1;

    bitsperplane = header.bitsperpixel * header.ncolplanes;
    if (bitsperplane != 8) {
        CTH_ERROR("Wrong number of bits per plane (%d).\n", bitsperplane);
        return NULL;
    }

    PCXEntry* new_pcx = new PCXEntry(name, "");

    new_pcx->width = x = header.xmax - header.xmin + 1;
    new_pcx->height = y = header.ymax - header.ymin + 1;
    if ((x > BUFF_WIDTH) || (y > BUFF_HEIGHT)) {
        CTH_WARN("PCX `%s' is %dx%d, larger than buffer %dx%d; image will be cropped.\n",
            name, x, y, BUFF_WIDTH, BUFF_HEIGHT);
    }
    new_pcx->data = buff = new unsigned char[x * y];

    zcompr = (header.compr == 1) ? 1 : 0;

    thisbyte = iread + 1;
    zdecomp = 0;

    for (int i = 0; i < y; i++) {
        int j;
        for (j = 0; j < x; j++) {
            *buff = getnextbyte();
            buff++;
        }
        for (; j < header.bytesperline; j++)
            getnextbyte();
    }

    /* read palette */
    if ((header.version == 2) || (header.version == 5)) {
        PaletteEntry* new_pal = new PaletteEntry(name, "");

        pal = (unsigned char*)new_pal->pal;

        zcompr = 0; /* palette is not compressed */
        if (getnextbyte() != 12) /* should be 12 */
            CTH_WARN("\n    Palette marker not found. Trying anyway.");

        for (y = 0; y < 768; y++)
            pal[y] = getnextbyte();

        CthughaBuffer::buffers[0].palette.add(new_pal);

        new_pcx->pal = CthughaBuffer::buffers[0].palette.getNEntries() - 1;

        CTH_DEBUG("\n    loaded palette %d from PCX", CthughaBuffer::buffers[0].palette.getNEntries());
    }

    return new_pcx;
}

int save_pcx(unsigned char* buffer, int width, int height, Palette pal) {
    FILE* file;
    headrec header;
    Palette tmp_pal;
    int i, j;
    const char* file_name = prtFileName("pcx");

    /* open file */
    if ((file = fopen(file_name, "w")) == NULL) {
        CTH_ERRNO(errno, "Can't open file for writing.");
        return 1;
    }

    /* fill header with information */
    header.id = 0x0A;
    header.version = 5;
    header.compr = 1;
    header.bitsperpixel = 8;
    header.xmin = 0;
    header.ymin = 0;
    header.xmax = width - 1;
    header.ymax = height - 1;
    header.horidpi = 0;
    header.vertdpi = 0;
    header.reserved = 0;
    header.ncolplanes = 1;
    header.bytesperline = width;
    header.greyscale = 1;
    for (i = 0; i < 16; i++)
        header.colormap[i][0] = header.colormap[i][1] = header.colormap[i][2] = 0;
    for (i = 0; i < 58; i++)
        header.filler[i] = 0;

    /* write header */
    if (fwrite(&header, sizeof(header), 1, file) != 1) {
        CTH_ERRNO(errno, "Can not write header.");
        fclose(file);
        return 1;
    }

    /* write buffer currently on screen */
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (*buffer == *(buffer + 1)) { /* repeating */
                unsigned char cnt = 0;
                while ((cnt < 62) && (*buffer == *(buffer + 1)) && (j < (width - 1)))
                    j++, cnt++, buffer++;
                cnt += 192 + 1;
                if (fputc(cnt, file) == EOF) { /* write count */
                    CTH_ERRNO(errno, "Can not write picture.");
                    fclose(file);
                    return 1;
                }
            } else { /* maybe repeat once */
                if (*buffer >= 192) {
                    if (fputc(193, file) == EOF) { /* repeat once */
                        CTH_ERRNO(errno, "Can not write picture.");
                        fclose(file);
                        return 1;
                    }
                }
            }
            if (fputc(*buffer, file) == EOF) { /* write data */
                CTH_ERRNO(errno, "Can not write picture.");
                fclose(file);
                return 1;
            }
            buffer++;
        }
    }

    /* create palette for save (in the future Palette might contains RGBA values) */
    for (i = 0; i < 256; i++)
        for (j = 0; j < 3; j++)
            tmp_pal[i][j] = pal[i][j];

    /* write palette marker */
    if (fputc(12, file) == EOF) {
        CTH_ERRNO(errno, "Can not write palette.");
        fclose(file);
        return 1;
    }
    /* write palette */
    if (fwrite(tmp_pal, 3 * 256, 1, file) != 1) {
        CTH_ERRNO(errno, "Can not write palette.");
        fclose(file);
        return 1;
    }

    /* close file */
    fclose(file);

    return 0;
}
