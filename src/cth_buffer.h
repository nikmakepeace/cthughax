#ifndef __CTH_BUFFER_H__
#define __CTH_BUFFER_H__

/*
 * definitions for table generation programs that need the
 * buffer size to be constant
 */

#if defined(CONST_BUFF) || defined(BUFF_WIDTH) || defined(BUFF_HEIGHT)

#ifndef BUFF_WIDTH
#define BUFF_WIDTH 320
#endif
#ifndef BUFF_HEIGHT
#define BUFF_HEIGHT 200
#endif

#define BUFF_BOTTOM (BUFF_HEIGHT - 1)
#define BUFF_SIZE (BUFF_WIDTH * BUFF_HEIGHT)

/*
 * Type of integer used for translation-files.
 *  32-bit interers used only if need.
 */
#if BUFF_SIZE > 65535
typedef int tint;
typedef unsigned int utint;
#else
typedef short tint;
typedef unsigned short utint;
#endif

#endif

#define MAX_BUFF_WIDTH 1024
#define MAX_BUFF_HEIGHT 1024

#endif
