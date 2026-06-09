// Indexed PCX image loading.

#include "cthugha.h"
#include "cth_buffer.h"
#include "display.h"
#include "Image.h"
#include "pcx.h"

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

class PcxByteReader {
    static const int ChunkSize = 8192;

    FILE* file;
    byte chunk[ChunkSize];
    int bytesRead;
    int position;
    int compressed;
    byte repeatValue;
    int repeatCount;

    int readRawByte() {
        if (position >= bytesRead) {
            if (feof(file)) {
                bytesRead = 0;
            } else {
                bytesRead = fread(chunk, 1, ChunkSize, file);
                if (ferror(file))
                    bytesRead = 0;
            }
            position = 0;
        }

        if (position >= bytesRead)
            return 0;

        return chunk[position++];
    }

public:
    PcxByteReader(FILE* file_, int compressed_)
        : file(file_)
        , bytesRead(0)
        , position(0)
        , compressed(compressed_)
        , repeatValue(0)
        , repeatCount(0) { }

    void setCompressed(int compressed_) {
        compressed = compressed_;
        repeatCount = 0;
    }

    byte nextByte() {
        if (repeatCount > 0) {
            repeatCount--;
            return repeatValue;
        }

        int value = readRawByte();
        if (compressed && value >= 192) {
            repeatCount = value & 0x3F;
            repeatValue = (byte)readRawByte();
            if (repeatCount > 0)
                repeatCount--;
            return repeatValue;
        }

        return (byte)value;
    }
};

IndexedImage* read_pcx_indexed_image(
    FILE* file, const char* name, const char* /* dir */, const char* /*total_name*/,
    const ImageLoadTarget& target) {
    int x, y;
    byte bitsperplane;
    headrec header;

    /* read header */
    int iread = fread(&header, 1, sizeof(header), file);
    if (iread != sizeof(header)) {
        CTH_ERROR("Can't read head of file: %s\n", name);
        return NULL;
    }

    CTH_DEBUG("pcx: version:%d, compr:%d, ncolpl.:%2d, greysc.:%d, X:%d-%d, Y:%d-%d\n",
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

    x = header.xmax - header.xmin + 1;
    y = header.ymax - header.ymin + 1;
    if (x <= 0 || y <= 0
        || ((unsigned long long)x * (unsigned long long)y > 1024ULL * 1024ULL * 64ULL)) {
        CTH_ERROR("PCX `%s' has unsupported dimensions %dx%d.\n", name, x, y);
        return NULL;
    }

    if ((x > target.width) || (y > target.height)) {
        CTH_WARN("PCX `%s' is %dx%d, larger than buffer %dx%d; image will be cropped.\n",
            name, x, y, target.width, target.height);
    }

    IndexedImage* image = new IndexedImage(name, x, y);
    unsigned char* buff = image->mutablePixels();
    PcxByteReader reader(file, header.compr == 1);

    for (int i = 0; i < y; i++) {
        int j;
        for (j = 0; j < x; j++) {
            *buff = reader.nextByte();
            buff++;
        }
        for (; j < header.bytesperline; j++)
            reader.nextByte();
    }

    ColorPalette* sourcePalette = 0;

    /* read palette */
    if ((header.version == 2) || (header.version == 5)) {
        sourcePalette = new ColorPalette();
        unsigned char* pal = (unsigned char*)sourcePalette->raw();

        reader.setCompressed(0); /* palette is not compressed */
        if (reader.nextByte() != 12) /* should be 12 */
            CTH_WARN("\n    Palette marker not found. Trying anyway.");

        for (y = 0; y < 768; y++)
            pal[y] = reader.nextByte();

        CTH_DEBUG("pcx: loaded source palette from `%s'\n", name);
    }

    image->setPalette(sourcePalette);
    return image;
}

EffectChoice* read_pcx_image(
    FILE* file, const char* name, const char* dir, const char* totalName,
    const ImageLoadTarget& target) {
    IndexedImage* image = read_pcx_indexed_image(file, name, dir, totalName,
        target);
    if (image == NULL)
        return NULL;

    return new ImageEntry(name, "", image);
}
