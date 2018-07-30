#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "disp_info.h"
struct fb_info {
	int fd;
	int xres;
	int yres;
	int bpp;
	char dev_name[32];
	unsigned int xoff;
	unsigned int  yoff;
	int smem_len;
	unsigned char *fb_mem;
	int padding[2];
};

extern struct fb_info * iq_lcd_init(char * dev_name);
extern void iq_lcd_deinit(struct fb_info *fb);
extern int iq_lcd_display_frame(unsigned char*frame, struct display_info *dis, struct fb_info *fb);
