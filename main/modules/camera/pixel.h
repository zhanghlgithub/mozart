#ifndef __PIXEL_H__
#define __PIXEL_H__
/*
struct bpp16_pixel {
	unsigned short red:5;
	unsigned short green:6;
	unsigned short blue:5;
};
*/
struct bpp16_pixel {
	unsigned short blue:5;
	unsigned short green:6;
	unsigned short red:5;
};

struct bpp24_pixel {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

struct bpp32_pixel {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
};

struct yuv422_sample {
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
	unsigned char b4;
};

struct yuv444_sample {
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
};

struct rgb888_sample {
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
};

struct rgb565_sample {
	unsigned short b1:5,
		b2:6,
		b3:5;
};

#endif /* __PIXEL_H__ */
