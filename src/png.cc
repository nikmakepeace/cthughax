// Indexed PNG image loader.

#include "cthugha.h"
#include "cth_buffer.h"
#include "Image.h"
#include "png.h"

#include <vector>
#include <zlib.h>

static unsigned int readPngUint32(const unsigned char* bytes) {
    return ((unsigned int)bytes[0] << 24)
        | ((unsigned int)bytes[1] << 16)
        | ((unsigned int)bytes[2] << 8)
        | (unsigned int)bytes[3];
}

class PngByteReader {
    FILE* file;

public:
    PngByteReader(FILE* file_)
        : file(file_) { }

    int readBytes(unsigned char* bytes, size_t size) {
        return fread(bytes, 1, size, file) == size;
    }

    int appendBytes(std::vector<unsigned char>& bytes, size_t size) {
        if (size == 0)
            return 1;

        size_t offset = bytes.size();
        bytes.resize(offset + size);
        return readBytes(&bytes[offset], size);
    }
};

static int pngScanlineBytes(int width, int bitDepth) {
    return (width * bitDepth + 7) / 8;
}

static unsigned char paethPredictor(unsigned char left, unsigned char up,
    unsigned char upLeft) {
    int p = int(left) + int(up) - int(upLeft);
    int pa = abs(p - int(left));
    int pb = abs(p - int(up));
    int pc = abs(p - int(upLeft));

    if (pa <= pb && pa <= pc)
        return left;
    if (pb <= pc)
        return up;
    return upLeft;
}

static int unfilterPngImage(const std::vector<unsigned char>& filtered,
    std::vector<unsigned char>& packedPixels, int width, int height, int bitDepth) {
    int rowBytes = pngScanlineBytes(width, bitDepth);
    int sourceStride = rowBytes + 1;
    int bytesPerPixel = 1;

    if ((int)filtered.size() != sourceStride * height)
        return 0;

    packedPixels.assign(rowBytes * height, 0);

    for (int y = 0; y < height; y++) {
        const unsigned char* source = &filtered[y * sourceStride];
        unsigned char* current = &packedPixels[y * rowBytes];
        const unsigned char* previous = (y > 0) ? &packedPixels[(y - 1) * rowBytes] : 0;
        int filter = source[0];
        source++;

        if (filter < 0 || filter > 4)
            return 0;

        for (int x = 0; x < rowBytes; x++) {
            unsigned char raw = source[x];
            unsigned char left = (x >= bytesPerPixel) ? current[x - bytesPerPixel] : 0;
            unsigned char up = (previous != 0) ? previous[x] : 0;
            unsigned char upLeft = (previous != 0 && x >= bytesPerPixel)
                ? previous[x - bytesPerPixel]
                : 0;
            unsigned char value = raw;

            switch (filter) {
            case 0:
                value = raw;
                break;
            case 1:
                value = (unsigned char)(raw + left);
                break;
            case 2:
                value = (unsigned char)(raw + up);
                break;
            case 3:
                value = (unsigned char)(raw + ((int(left) + int(up)) / 2));
                break;
            case 4:
                value = (unsigned char)(raw + paethPredictor(left, up, upLeft));
                break;
            }

            current[x] = value;
        }
    }

    return 1;
}

static unsigned char indexedPngPixel(const unsigned char* row, int x, int bitDepth) {
    switch (bitDepth) {
    case 8:
        return row[x];
    case 4:
        return (row[x / 2] >> ((x & 1) ? 0 : 4)) & 0x0F;
    case 2:
        return (row[x / 4] >> (6 - 2 * (x & 3))) & 0x03;
    case 1:
        return (row[x / 8] >> (7 - (x & 7))) & 0x01;
    default:
        return 0;
    }
}

IndexedImage* read_png_indexed_image(
    FILE* file, const char* name, const char* /* dir */, const char* /*total_name*/,
    const ImageLoadTarget& target) {
    static const unsigned char pngSignature[8] = {
        137, 80, 78, 71, 13, 10, 26, 10
    };
    unsigned char signature[8];
    int width = 0;
    int height = 0;
    int bitDepth = 0;
    int colorType = -1;
    int compressionMethod = -1;
    int filterMethod = -1;
    int interlaceMethod = -1;
    int sawHeader = 0;
    int sawPalette = 0;
    int sawEnd = 0;
    ColorPalette* sourcePalette = new ColorPalette();
    std::vector<unsigned char> idat;
    PngByteReader reader(file);

    if (!reader.readBytes(signature, sizeof(signature))
        || memcmp(signature, pngSignature, sizeof(signature)) != 0) {
        CTH_ERROR("Illegal PNG signature in file: %s\n", name);
        delete sourcePalette;
        return NULL;
    }

    while (!sawEnd) {
        unsigned char lengthBytes[4];
        unsigned char type[4];
        unsigned char crc[4];

        if (!reader.readBytes(lengthBytes, sizeof(lengthBytes)))
            break;
        if (!reader.readBytes(type, sizeof(type)))
            break;

        unsigned int length = readPngUint32(lengthBytes);
        if (length > 64U * 1024U * 1024U) {
            CTH_ERROR("PNG chunk too large in file: %s\n", name);
            delete sourcePalette;
            return NULL;
        }

        std::vector<unsigned char> data;
        if (!reader.appendBytes(data, length) || !reader.readBytes(crc, sizeof(crc))) {
            CTH_ERROR("Can't read PNG chunk in file: %s\n", name);
            delete sourcePalette;
            return NULL;
        }

        if (memcmp(type, "IHDR", 4) == 0) {
            if (length != 13) {
                CTH_ERROR("Illegal PNG header length in file: %s\n", name);
                delete sourcePalette;
                return NULL;
            }

            width = (int)readPngUint32(&data[0]);
            height = (int)readPngUint32(&data[4]);
            bitDepth = data[8];
            colorType = data[9];
            compressionMethod = data[10];
            filterMethod = data[11];
            interlaceMethod = data[12];
            sawHeader = 1;

            CTH_DEBUG("png: width=%d height=%d bit-depth=%d color-type=%d interlace=%d\n",
                width, height, bitDepth, colorType, interlaceMethod);
        } else if (memcmp(type, "PLTE", 4) == 0) {
            if (length == 0 || (length % 3) != 0 || length > 768) {
                CTH_ERROR("Illegal PNG palette length in file: %s\n", name);
                delete sourcePalette;
                return NULL;
            }

            sourcePalette->clear();
            for (unsigned int i = 0; i < length / 3; i++) {
                sourcePalette->setColor(i, data[i * 3], data[i * 3 + 1],
                    data[i * 3 + 2]);
            }
            sawPalette = 1;
        } else if (memcmp(type, "IDAT", 4) == 0) {
            idat.insert(idat.end(), data.begin(), data.end());
        } else if (memcmp(type, "IEND", 4) == 0) {
            sawEnd = 1;
        }
    }

    if (!sawHeader || !sawEnd || !sawPalette || idat.empty()) {
        CTH_ERROR("Incomplete indexed PNG file: %s\n", name);
        delete sourcePalette;
        return NULL;
    }

    if (colorType != 3) {
        CTH_ERROR("PNG `%s' is color type %d; only indexed PNG is supported.\n",
            name, colorType);
        delete sourcePalette;
        return NULL;
    }

    if (bitDepth != 1 && bitDepth != 2 && bitDepth != 4 && bitDepth != 8) {
        CTH_ERROR("PNG `%s' has unsupported indexed bit depth %d.\n", name, bitDepth);
        delete sourcePalette;
        return NULL;
    }

    if (compressionMethod != 0 || filterMethod != 0 || interlaceMethod != 0) {
        CTH_ERROR("PNG `%s' uses unsupported compression/filter/interlace settings.\n", name);
        delete sourcePalette;
        return NULL;
    }

    if (width <= 0 || height <= 0
        || ((unsigned long long)width * (unsigned long long)height > 1024ULL * 1024ULL * 64ULL)) {
        CTH_ERROR("PNG `%s' has unsupported dimensions %dx%d.\n", name, width, height);
        delete sourcePalette;
        return NULL;
    }

    if ((width > target.width) || (height > target.height)) {
        CTH_WARN("PNG `%s' is %dx%d, larger than buffer %dx%d; image will be cropped.\n",
            name, width, height, target.width, target.height);
    }

    int rowBytes = pngScanlineBytes(width, bitDepth);
    uLongf filteredSize = (uLongf)((rowBytes + 1) * height);
    std::vector<unsigned char> filtered(filteredSize);
    int zlibResult = uncompress(&filtered[0], &filteredSize, &idat[0], (uLong)idat.size());
    if (zlibResult != Z_OK || filteredSize != (uLongf)((rowBytes + 1) * height)) {
        CTH_ERROR("Can not inflate PNG image data in file: %s\n", name);
        delete sourcePalette;
        return NULL;
    }

    std::vector<unsigned char> packedPixels;
    if (!unfilterPngImage(filtered, packedPixels, width, height, bitDepth)) {
        CTH_ERROR("Can not unfilter PNG image data in file: %s\n", name);
        delete sourcePalette;
        return NULL;
    }

    IndexedImage* image = new IndexedImage(name, width, height, sourcePalette);
    unsigned char* pixels = image->mutablePixels();
    for (int y = 0; y < height; y++) {
        const unsigned char* row = &packedPixels[y * rowBytes];
        for (int x = 0; x < width; x++)
            pixels[y * width + x] = indexedPngPixel(row, x, bitDepth);
    }

    return image;
}

EffectChoice* read_png_image(
    FILE* file, const char* name, const char* dir, const char* totalName,
    const ImageLoadTarget& target) {
    IndexedImage* image = read_png_indexed_image(file, name, dir, totalName,
        target);
    if (image == NULL)
        return NULL;

    return new ImageEntry(name, "", image);
}
