// Built-in translation table generators.

#include "cthugha.h"
#include "ProcessServices.h"
#include "TranslateGenerator.h"

#include <math.h>
#include <stdint.h>

static const double PI = 3.14159265358979323846;

class GeneratorRandom {
    uint32_t state;

public:
    GeneratorRandom(unsigned int seed)
        : state(seed ? seed : 1) { }

    int next(int range) {
        if (range <= 0)
            return 0;

        state = state * 1664525u + 1013904223u;
        return (int)(((uint64_t)state * (uint32_t)range) >> 32);
    }
};

static int foldedIndex(int x, int y, const TranslateGenerationTarget& target) {
    return abs(x + y * target.width) % target.size;
}

static int clampedSource(int x, int y, const TranslateGenerationTarget& target) {
    if (y >= target.height || y < 0 || x >= target.width || x < 0)
        return 0;
    return y * target.width + x;
}

static int clampInt(int value, int low, int high) {
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

static unsigned int randomSeedBase(RandomSource& randomSource) {
    unsigned int high = (unsigned int)randomSource.uniformInt(65536);
    unsigned int low = (unsigned int)randomSource.uniformInt(65536);
    unsigned int seed = (high << 16) ^ low;
    return seed != 0 ? seed : 1;
}

TranslateGeneratorOptions::TranslateGeneratorOptions()
    : speed(100)
    , speedJitterPercent(70)
    , offsetJitterPercent(70)
    , reverse(0)
    , slowX(0)
    , slowY(1)
    , spiralCount(1)
    , yinYang(0)
    , yyWidth(10.0)
    , deltaR(2.0)
    , deltaA(0.1)
    , seed(1)
    , randomizeSeed(0) { }

GeneratedTranslationTable::GeneratedTranslationTable()
    : id()
    , description()
    , table()
    , width(0)
    , height(0) { }

TranslationCatalog::Entry::Entry(const char* id_, const char* description_,
    const TranslateGenerator& generator_, const TranslateGeneratorOptions& options_)
    : id(id_ != 0 ? id_ : "")
    , description(description_ != 0 ? description_ : "")
    , generator(&generator_)
    , options(options_) { }

void TranslationCatalog::add(const char* id, const char* description,
    const TranslateGenerator& generator, const TranslateGeneratorOptions& options) {
    entries.push_back(Entry(id, description, generator, options));
}

void TranslationCatalog::generateAll(int width, int height, RandomSource& randomSource,
    std::vector<GeneratedTranslationTable>& tables) const {
    TranslateGenerationTarget target(width, height);
    tables.clear();
    tables.reserve(entries.size());

    unsigned int seedBase = randomSeedBase(randomSource);

    for (unsigned int i = 0; i < entries.size(); i++) {
        TranslateGeneratorOptions options = entries[i].options;
        if (options.randomizeSeed)
            options.seed = seedBase ^ (0x9e3779b9u * (i + 1));

        GeneratedTranslationTable table;
        table.id = entries[i].id;
        table.description = entries[i].description;
        table.width = width;
        table.height = height;
        entries[i].generator->generate(target, options, table.table);
        tables.push_back(table);
    }
}

/*
 * Ported from cmd_bighalfwheel.c.
 * Source provenance: minor Linux/cthugha-L changes by Harald Deischinger.
 */
class BigHalfWheelTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        (void)options;

        table.assign(target.size, 0);

        int cx = (int)(target.width * 0.4);
        int cy = 0;
        double q = PI / 2.0;
        double p = 0.0;

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                int dx;
                int dy;

                if (y == 0 || y == target.height) {
                    dx = (int)((double)(cx - x) * 0.75);
                    dy = cy - y;
                } else {
                    int dist = (int)sqrt((double)((x - cx) * (x - cx) + (y - cy) * (y - cy)));
                    double ang;

                    if (x == cx) {
                        if (y > cy)
                            ang = q;
                        else
                            ang = -q;
                    } else
                        ang = atan((double)(y - cy) / (double)(x - cx));

                    if (x < cx)
                        ang += PI;

                    if (dist < target.height) {
                        dx = (int)ceil(-sin(ang - p) * dist / 10.0);
                        dy = (int)ceil(cos(ang - p) * dist / 10.0);
                    } else {
                        dx = (x < cx) ? 3 : -3;
                        dy = 0;
                    }

                    if (x == 0 || x == target.width) {
                        dx = cx - x;
                        dy = (int)((double)(cy - y) * 0.75);
                    }
                }

                table[x + y * target.width] = foldedIndex(x + dx, y + dy, target);
            }
        }
    }
};

/*
 * Ported from cmd_downspiral.c.
 * Source provenance: minor Linux/cthugha-L changes by Harald Deischinger.
 */
class DownSpiralTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        (void)options;

        table.assign(target.size, 0);

        int cx = target.width / 2;
        int cy = target.height / 2;
        double q = PI / 2.0;
        double p = 45.0 / 180.0 * PI;

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                int dx;
                int dy;

                if (y == 0 || y == target.height) {
                    dx = (int)((double)(cx - x) * 0.75);
                    dy = cy - y;
                } else {
                    int dist = (int)sqrt((double)((x - cx) * (x - cx) + (y - cy) * (y - cy)));
                    double ang;

                    if (x == cx) {
                        if (y > cy)
                            ang = q;
                        else
                            ang = -q;
                    } else
                        ang = atan((double)(y - cy) / (double)(x - cx));

                    if (x < cx)
                        ang += PI;

                    dx = (int)ceil(-sin(ang - p) * dist / 10.0);
                    dy = (int)ceil(cos(ang - p) * dist / 10.0);

                    if (x == 0 || x == target.width) {
                        dx = cx - x;
                        dy = (int)((double)(cy - y) * 0.75);
                    }
                }

                table[x + y * target.width] = foldedIndex(x + dx, y + dy, target);
            }
        }
    }
};

/*
 * Ported from cmd_gentable.c, based on mkgentable.
 * Source provenance: Cthugha - Audio Seeded Image Processing;
 * Zaph, Digital Aasvogel Group, Torps Productions 1993-1994.
 * Linux/cthugha-L changes by Harald Deischinger.
 */
class GenericSpiralTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        const int maxSpirals = 64;
        int centersX[maxSpirals];
        int centersY[maxSpirals];
        int dir[maxSpirals];
        int spiralCount = options.spiralCount;
        GeneratorRandom random(options.seed);

        if (spiralCount == 0) {
            centersX[0] = target.width / 2;
            centersY[0] = target.height / 2;
            dir[0] = 1;
            spiralCount = 1;
        } else {
            spiralCount = clampInt(spiralCount, 1, maxSpirals);
            for (int i = 0; i < spiralCount; i++) {
                centersX[i] = random.next(target.width);
                centersY[i] = random.next(target.height);
                dir[i] = random.next(5) - 2;
            }
        }

        table.assign(target.size, 0);

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                double mappedX = 0.0;
                double mappedY = 0.0;
                double totalWeight = 0.0;

                for (int i = 0; i < spiralCount; i++) {
                    double centerY = centersY[i];
                    double centerX = centersX[i];
                    double tempX = fabs(x - centerX);
                    double tempY = fabs(y - centerY);
                    double polarR = sqrt(tempX * tempX + tempY * tempY);
                    double polarA = atan2((double)(x - centerX), (double)(y - centerY));

                    polarR += (options.deltaR + random.next(10) * 0.01) * (double)dir[i];
                    if (polarR < 0)
                        polarR = 0.0;

                    if (options.yinYang) {
                        polarA -= options.deltaA * 3.0
                            * (double)(5 - (int)(polarR / 11.0) % 11) / 5.0;

                        if (((int)(polarR / options.yyWidth) % 2))
                            polarA += options.deltaA;
                        else
                            polarA -= options.deltaA;
                    } else {
                        polarA += (options.deltaA + random.next(10) * 0.01) * (double)dir[i];
                    }

                    double sourceY = polarR * cos(polarA) + centerY;
                    double sourceX = polarR * sin(polarA) + centerX;
                    double dist = sqrt(((double)x - centerX) * ((double)x - centerX)
                        + ((double)y - centerY) * ((double)y - centerY));
                    double weight = 1.0 / (dist * dist + 1.0);

                    mappedX += weight * sourceX;
                    mappedY += weight * sourceY;
                    totalWeight += weight;
                }

                int mapX = (int)(mappedX / totalWeight);
                int mapY = (int)(mappedY / totalWeight);
                table[x + y * target.width] = clampedSource(mapX, mapY, target);
            }
        }
    }
};

/*
 * Ported from cmd_huricn.c, based on mkhuricn.
 * Source provenance: hurricane translation table by Ofer Faigon,
 * September 1994, ofer@brm.co.il / oferf@itexjct.jct.ac.il.
 * Linux/cthugha-L changes by Harald Deischinger.
 */
class HurricaneTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        GeneratorRandom random(options.seed);
        table.assign(target.size, 0);

        int xCenter = target.width / 3;
        int yCenter = target.height / 2 - 10;
        int speed = clampInt(options.speed, 30, 300);
        int speedJitterPercent = clampInt(options.speedJitterPercent, 0, 100);

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                long sp;
                if (speedJitterPercent == 0)
                    sp = speed;
                else {
                    int speedFactor = random.next(speedJitterPercent + 1)
                        - speedJitterPercent / 3;
                    sp = speed * (100L + speedFactor) / 100L;
                }

                int dx = x - xCenter;
                int dy = y - yCenter;

                if (options.slowX || options.slowY) {
                    long dSquared = (long)dx * dx + (long)dy * dy + 1;
                    if (options.slowY)
                        dx = (int)(dx * 2500L / dSquared);
                    if (options.slowX)
                        dy = (int)(dy * 2500L / dSquared);
                }

                if (options.reverse)
                    sp = -sp;

                int mapX = (int)(x + (dy * sp) / 700);
                int mapY = (int)(y - (dx * sp) / 700);

                while (mapY < 0)
                    mapY += target.height;
                while (mapX < 0)
                    mapX += target.width;
                mapY %= target.height;
                mapX %= target.width;

                table[x + y * target.width] = mapY * target.width + mapX;
            }
        }
    }
};

/*
 * Ported from cmd_randswirls.c.
 * Source provenance: minor Linux/cthugha-L changes by Harald Deischinger.
 */
class RandomSwirlsTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        GeneratorRandom random(options.seed);
        table.assign(target.size, 0);

        double stepInc = (double)target.width / 2500.0;
        int lineLength = (int)((target.width < target.height ? target.width : target.height) * 0.35);
        int centerLine = lineLength / 2;
        int dist = (stepInc > 0.0) ? (int)(8.0 / stepInc) : 0;
        if (lineLength <= 0 || dist <= 0)
            return;

        class Point {
        public:
            int x;
            int y;
        };

        std::vector<Point> line(lineLength * dist);
        for (int j = 0; j < dist; j++) {
            for (int k = 0; k < lineLength; k++) {
                line[j * lineLength + k].x = 0;
                line[j * lineLength + k].y = k;
            }
        }

        for (int y = 0; y < target.height; y++)
            for (int x = 0; x < target.width; x++)
                table[x + y * target.width] = abs(x - 3 + y * target.width) % target.size;

        double cx = 0;
        double cy = centerLine;
        double angle = 0;
        int turn = random.next(20);
        int cursor = 0;

        for (int k = 0; k < 100000; k++) {
            if (random.next(50) == 1)
                turn = random.next(20) - 10;

            angle += (double)turn / 360.0 * stepInc;
            double s = sin(angle) * stepInc;
            double c = cos(angle) * stepInc;
            cx += s;
            cy += c;

            for (int i = 0; i < lineLength; i++) {
                Point& current = line[(cursor % dist) * lineLength + i];
                current.x = (int)(cx + (centerLine - i) * c) % target.width;
                current.y = (int)(cy + (centerLine - i) * s) % target.height;

                Point& previous = line[((cursor + 1) % dist) * lineLength + i];
                int dx = previous.x - current.x;
                int dy = previous.y - current.y;

                table[abs(current.x + current.y * target.width) % target.size]
                    = abs(current.x + dx + (current.y + dy) * target.width) % target.size;
                cursor++;
            }
        }

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                if (y == 0 || y == target.height) {
                    int dx = (int)((cx - x) * 0.75);
                    int dy = (int)(cy - y);
                    table[x + y * target.width] = foldedIndex(x + dx, y + dy, target);
                } else if (x == 0 || x == target.width) {
                    int dx = (int)(cx - x);
                    int dy = (int)((cy - y) * 0.75);
                    table[x + y * target.width] = foldedIndex(x + dx, y + dy, target);
                }
            }
        }
    }
};

/*
 * Ported from cmd_smoke.c, based on mksmoke.
 * Source provenance: smoke translation table by Ofer Faigon,
 * September 1994, ofer@brm.co.il / oferf@itexjct.jct.ac.il.
 * Linux/cthugha-L changes by Harald Deischinger.
 */
class SmokeTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        GeneratorRandom random(options.seed);
        table.assign(target.size, 0);

        int speed = clampInt(options.speed, 30, 300);
        int offsetJitterPercent = clampInt(options.offsetJitterPercent, 0, 100);
        int offsetRange = 12 * offsetJitterPercent / 100;
        int maxOffset = (5 + ((offsetRange > 0) ? offsetRange - 1 : 0)) * speed / 100;

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                int mapX = x - (5 + random.next(offsetRange)) * speed / 100;
                int mapY = y - (5 + random.next(offsetRange)) * speed / 100;
                // Do not pull edge pixels inward; outside the framebuffer is black.
                if (mapX < maxOffset || mapY < maxOffset)
                    table[x + y * target.width] = 0;
                else
                    table[x + y * target.width] = clampedSource(mapX, mapY, target);
            }
        }
    }
};

/*
 * Ported from cmd_space.c, based on mkspace.
 * Source provenance: space-flight translation table by Ofer Faigon,
 * September 1994, ofer@brm.co.il / oferf@itexjct.jct.ac.il.
 * Linux/cthugha-L changes by Harald Deischinger.
 */
class SpaceTranslateGenerator : public TranslateGenerator {
public:
    void generate(const TranslateGenerationTarget& target,
        const TranslateGeneratorOptions& options,
        std::vector<int>& table) const {
        GeneratorRandom random(options.seed);
        table.assign(target.size, 0);

        int speed = clampInt(options.speed, 30, 300);
        int speedJitterPercent = clampInt(options.speedJitterPercent, 0, 100);
        int centerX = target.width / 2;
        int centerY = target.height / 2;

        for (int y = 0; y < target.height; y++) {
            for (int x = 0; x < target.width; x++) {
                int dx = x - centerX;
                int dy = y - centerY;
                int mapX;
                int mapY;

                if (!options.reverse && abs(dx) < 30 && abs(dy) < 20
                    && random.next(abs(dx) + abs(dy)) < 4) {
                    mapX = random.next(target.width);
                    mapY = random.next(target.height);
                } else {
                    long sp;
                    if (speedJitterPercent == 0)
                        sp = speed;
                    else {
                        int speedFactor = random.next(speedJitterPercent + 1)
                            - speedJitterPercent / 3;
                        sp = speed * (100L + speedFactor) / 100L;
                    }

                    if (options.reverse)
                        sp = -sp;

                    mapX = (int)(x - (dx * sp) / 700);
                    mapY = (int)(y - (dy * sp) / 700);
                }

                table[x + y * target.width] = clampedSource(mapX, mapY, target);
            }
        }
    }
};

static TranslateGeneratorOptions randomizeSeed() {
    TranslateGeneratorOptions options;
    options.randomizeSeed = 1;
    return options;
}

static TranslateGeneratorOptions genericOptions(int spiralCount, double yyWidth,
    double deltaR, double deltaA, int yinYang) {
    TranslateGeneratorOptions options = randomizeSeed();
    options.spiralCount = spiralCount;
    options.yyWidth = yyWidth;
    options.deltaR = deltaR;
    options.deltaA = deltaA;
    options.yinYang = yinYang;
    return options;
}

static TranslateGeneratorOptions spaceOptions(int speed, int speedJitterPercent) {
    TranslateGeneratorOptions options;
    options.speed = speed;
    options.speedJitterPercent = speedJitterPercent;
    return options;
}

const TranslationCatalog& defaultTranslationCatalog() {
    static BigHalfWheelTranslateGenerator bigHalfWheel;
    static DownSpiralTranslateGenerator downSpiral;
    static GenericSpiralTranslateGenerator genericSpiral;
    static HurricaneTranslateGenerator hurricane;
    static RandomSwirlsTranslateGenerator randomSwirls;
    static SmokeTranslateGenerator smoke;
    static SpaceTranslateGenerator space;
    static TranslationCatalog catalog;
    static int initialized = 0;

    if (!initialized) {
        catalog.add("bighalfwheel", "Krusty wheel 1", bigHalfWheel);
        catalog.add("downspiral", "Krusty wheel 2", downSpiral);
        catalog.add("gentable", "Black holes", genericSpiral,
            genericOptions(10, 10.0, 2.0, 0.1, 0));
        catalog.add("hurricane", "Hurricane/Jupiters Eye", hurricane, randomizeSeed());
        catalog.add("randswirls", "Vines", randomSwirls, randomizeSeed());
        catalog.add("rotate", "Rotate", genericSpiral,
            genericOptions(0, 10.0, 2.0, 0.1, 0));
        catalog.add("smoke", "Falling smoke", smoke);
        catalog.add("space-fast", "Fast space flight", space);
        catalog.add("space-slow", "Slow space flight", space,
            spaceOptions(30, 10));
        catalog.add("yin-yang", "Yin yang", genericSpiral,
            genericOptions(0, 60.0, 0.0, -0.025, 1));
        initialized = 1;
    }

    return catalog;
}
