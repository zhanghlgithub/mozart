
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include <linux/videodev2.h>
#include "lcd.h"
#include "pixel.h"
#include "jzmedia.h"
#include "cim.h"
#include "disp_info.h"
#include "convert.h"
struct fb_info * iq_lcd_init(char * dev_name)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	struct fb_info *fb_info;

	fb_info = (struct fb_info *)malloc(sizeof(struct fb_info));
	if(fb_info == NULL){
		fprintf(stderr, "Error: cannot open framebuffer device.\n");
		return NULL;
	}
	strcpy(fb_info->dev_name,dev_name);
	// Open the file for reading and writing
	fb_info->fd = open(fb_info->dev_name, O_RDWR);
	if (!fb_info->fd) {
		fprintf(stderr, "Error: cannot open framebuffer device.\n");
		goto error;
	}
	// Get fixed screen information
	if (ioctl(fb_info->fd, FBIOGET_FSCREENINFO, &finfo)) {
		fprintf(stderr, "Error reading fixed information.\n");
		goto error;
	}

	// Get variable screen information
	if (ioctl(fb_info->fd, FBIOGET_VSCREENINFO, &vinfo)) {
		fprintf(stderr, "Error reading variable information.\n");
		goto error;
	}

	// Put variable screen information
	vinfo.activate |= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;

	// Figure out the size of the screen in bytes
	fb_info->xres = vinfo.xres;
	fb_info->yres = vinfo.yres;
	fb_info->bpp = vinfo.bits_per_pixel;
	fb_info->smem_len = finfo.smem_len;

	fprintf(stdout, "lcd fb info: xres=%d, yres=%d bpp=%d, smem_len=%d\n",
		vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, finfo.smem_len);

	// Map the device to memory
	fb_info->fb_mem = (unsigned char *)mmap(0,
				       finfo.smem_len,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED,
				       fb_info->fd,
				       0);
	fprintf(stdout, "fbp = %p\n", fb_info->fb_mem);
	if (!fb_info->fb_mem) {
		fprintf(stderr, "Error: failed to map framebuffer device to memory.\n");
		goto error;
	}
	fprintf(stdout, "Open %s successfully.\n", fb_info->dev_name);
	return fb_info;
error:
	free(fb_info);
	return NULL;
}

void iq_lcd_deinit(struct fb_info *fb)
{
	if (fb->fb_mem) {
		munmap(fb->fb_mem, fb->smem_len);
		fb->fb_mem = NULL;
	}
	if (fb->fd > 0) {
		close(fb->fd);
		fb->fd = -1;
	}
	free(fb);
	fb = NULL;
}

static void fb_set_a_pixel(struct fb_info *fb_info, int line, int col,
		    unsigned char red, unsigned char green, unsigned char blue,
		    unsigned char alpha)
{
	switch (fb_info->bpp) {
	case 16:
		{
			struct bpp16_pixel *pixel = (struct bpp16_pixel *)fb_info->fb_mem;
			pixel[line * fb_info->xres + col].red = (red >> 3);
			pixel[line * fb_info->xres + col].green = (green >> 2);
			pixel[line * fb_info->xres + col].blue = (blue >> 3);
		}
		break;
	case 24:
		{
			struct bpp24_pixel *pixel = (struct bpp24_pixel *)fb_info->fb_mem;
			pixel[line * fb_info->xres + col].red = red;
			pixel[line * fb_info->xres + col].green = green;
			pixel[line * fb_info->xres + col].blue = blue;
		}
		break;

	case 32:
		{
			struct bpp32_pixel *pixel = (struct bpp32_pixel *)fb_info->fb_mem;
			pixel[line * fb_info->xres + col].red = red;
			pixel[line * fb_info->xres + col].green = green;
			pixel[line * fb_info->xres + col].blue = blue;
			pixel[line * fb_info->xres + col].alpha = alpha;
		}
		break;
	default:
		break;
	}
}

void window_cut(struct display_info *dis, struct fb_info *fb)
{
	dis->xoff = 0;
	dis->yoff = 0;
	fb->xoff = 0;
	fb->yoff = 0;
		
	if (dis->width > fb->xres)
		 dis->xoff = (dis->width - fb->xres) / 2;
	 else if (dis->width < fb->xres)
		 fb->xoff = (fb->xres - dis->width) / 2;
	 else
		 dis->xoff = fb->xoff = 0;

	  if (dis->height > fb->yres)
		  dis->yoff = (dis->height - fb->yres) / 2;
	  else if (dis->height < fb->yres)
		  fb->yoff = (fb->yres - dis->height) / 2;
	  else
		  dis->yoff = fb->yoff = 0;
}

int display_direct_to_fb(unsigned char *frame, struct display_info *dis, struct fb_info *fb)
{
	int format = dis->fmt.format;
	int line, col;
	int fb_line;
	int fb_col;
	int colorR, colorG, colorB;
	struct yuv422_sample yuv422_samp;
	struct yuv422_sample *yuv422_pixel =  NULL;
	unsigned char y0, y1, u, v;
	int ret = -1;
	window_cut(dis, fb);
	for (line = dis->yoff, fb_line = fb->yoff;
			( (line < (dis->yoff + dis->height)) && (fb_line < (fb->yoff + fb->yres)) );
			line ++, fb_line++)
	{
		yuv422_pixel = (struct yuv422_sample *)
				(frame + line * dis->width * 2 + dis->xoff * 2);
		for (col = dis->xoff, fb_col = fb->xoff;
				( (col < (dis->xoff + dis->width)) && (fb_col < (fb->yoff + fb->xres)) );
				col += 2, fb_col += 2) {
			yCbCr422_normalization(format, yuv422_pixel, &yuv422_samp);
			y0 = yuv422_samp.b1;
			v = yuv422_samp.b2;
			y1 = yuv422_samp.b3;
			u = yuv422_samp.b4;
			
			yuv422_pixel ++;
			
			colorB = y0 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y0 -((179 * (v - 128) +86 * (u - 128)) >> 8);
			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y0 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			fb_set_a_pixel(fb, fb_line, fb_col, colorR, colorG, colorB, 0);

			colorB = y1 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y1 - ((179 * (v - 128) + 86 * (u - 128)) >> 8);
			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y1 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);
			
			fb_set_a_pixel(fb, fb_line, fb_col + 1, colorR, colorG, colorB, 0);

		}
	}
	ret = 0;
	return ret;
}
#if 0
static int display_to_fb(unsigned char *frame, struct display_info *dis, struct fb_info *fb)
{
	int src_fourcc = dis->fmt.fourcc;
	int src_height = dis->height;
	int src_width = dis->width;

	int dst_fourcc = fb->bpp;
	if(dst_fourcc == 16)
	{
		switch (src_fourcc)
		{
		   case V4L2_PIX_FMT_RGB24 :
			   printf("V4L2_PIX_FMT_RGB24 \n");
			   break;
		   case V4L2_PIX_FMT_RGB565 :
			   printf("V4L2_PIX_FMT_RGB565 \n");
			   break;
		   case V4L2_PIX_FMT_YUYV :
			   YUYV_to_RGB565 (frame, fb->fb_mem, src_height , src_width,  fb->xres*32);
			   break;
		   case V4L2_PIX_FMT_YVYU :
			   break;
		}
	}
	else
	{
		switch (src_fourcc)
		{
		   case V4L2_PIX_FMT_RGB24 :
			   printf("V4L2_PIX_FMT_RGB24 \n");
			   break;
		   case V4L2_PIX_FMT_RGB565 :
			   printf("V4L2_PIX_FMT_RGB565 \n");
			   break;
		   case V4L2_PIX_FMT_YUYV :
			   YUYV_to_RGB888 (frame, fb->fb_mem, src_height , src_width,  fb->xres*32);
			   break;
		   case V4L2_PIX_FMT_YVYU :
			   break;
		}
	}
	return 0;

}
#endif
int iq_lcd_display_frame(unsigned char*frame, struct display_info *dis, 
		struct fb_info *fb)
{
	int ret = -1;
	ret = display_direct_to_fb(frame,dis,fb);
	return ret;
}
