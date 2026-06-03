#ifndef __DISP_SYS_H
#define __DISP_SYS_H

#include "display.h"

int init_display_sys();
unsigned char* pre_draw_sys();
int tile_buffer();
int disp_copy_sys(int);
int disp_clear_box_sys(int x, int y, int width, int height);
int disp_copy_box_sys(int x, int y, int width, int height, int destx, int desty);

int display_print_string_sys(int x, int y, const char* text, int color, int len);

int init_palettes_sys();
int exit_palettes_sys();
int set_palette_sys();
int display_sync_sys();
extern Palette cur_palette;
extern int colormapped;
extern unsigned char* display_bitmap;

#endif
