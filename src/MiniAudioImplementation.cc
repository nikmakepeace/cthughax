/** @file
 * Single miniaudio implementation translation unit.
 */

#include "config.h"

#if WITH_MINIAUDIO == 1

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MA_NO_NODE_GRAPH
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_AUDIO4
#define MA_NO_OSS
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#endif
