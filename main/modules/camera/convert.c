
/* convert any format to Y0CbY1Cr */
#include <stdio.h>
#include "pixel.h"
#include "disp_info.h"
#include "cim.h"
void yCbCr422_normalization(unsigned int fmt, struct yuv422_sample *yuv422_samp_in,
			  struct yuv422_sample *yuv422_samp_out)
{
	unsigned char y0 = 0;
	unsigned char cb = 0;
	unsigned char y1 = 0;
	unsigned char cr = 0;
	switch(fmt & YCbCr422_MASK) {
	case YCbCr422_Y0CbY1Cr:
		y0 = yuv422_samp_in->b1;
		cb = yuv422_samp_in->b2;
		y1 = yuv422_samp_in->b3;
		cr = yuv422_samp_in->b4;
		break;
	case YCbCr422_CbY1CrY0:
		cb = yuv422_samp_in->b1;
		y1 = yuv422_samp_in->b2;
		cr = yuv422_samp_in->b3;
		y0 = yuv422_samp_in->b4;
		break;
	case YCbCr422_Y1CrY0Cb:
		y1 = yuv422_samp_in->b1;
		cr = yuv422_samp_in->b2;
		y0 = yuv422_samp_in->b3;
		cb = yuv422_samp_in->b4;
		break;
	case YCbCr422_CrY0CbY1:
		y1 = yuv422_samp_in->b1;
		cr = yuv422_samp_in->b1;
		y0 = yuv422_samp_in->b2;
		cb = yuv422_samp_in->b3;
		y1 = yuv422_samp_in->b4;
		break;
	case YCbCr422_CrY1CbY0:
		cr = yuv422_samp_in->b1;
		y1 = yuv422_samp_in->b2;
		cb = yuv422_samp_in->b3;
		y0 = yuv422_samp_in->b4;
		break;
	case YCbCr422_Y1CbY0Cr:
		y1 = yuv422_samp_in->b1;
		cb = yuv422_samp_in->b2;
		y0 = yuv422_samp_in->b3;
		cr = yuv422_samp_in->b4;
		break;
	case YCbCr422_CbY0CrY1:
		cb = yuv422_samp_in->b1;
		y0 = yuv422_samp_in->b2;
		cr = yuv422_samp_in->b3;
		y1 = yuv422_samp_in->b4;
		break;
	case YCbCr422_Y0CrY1Cb:
		y0 = yuv422_samp_in->b1;
		cr = yuv422_samp_in->b2;
		y1 = yuv422_samp_in->b3;
		cb = yuv422_samp_in->b4;
		break;
	default:
		fprintf(stderr, "%s() L%d: unsupported YUV422 format!\n", __func__, __LINE__);
		return;
	}
	yuv422_samp_out->b1 = y0;
	yuv422_samp_out->b2 = cb;
	yuv422_samp_out->b3 = y1;
	yuv422_samp_out->b4 = cr;
}

void rgb888_normalization(unsigned int fmt, struct rgb888_sample *in_samp,
			  struct rgb888_sample *out_samp)
{
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	if (!DF_IS_RGB888(fmt))
		return;

	switch(fmt & RAW888_MASK) {
	case RAW888_GRB:
		g = in_samp->b1;
		r = in_samp->b2;
		b = in_samp->b3;
		break;
	case RAW888_GBR:
		g = in_samp->b1;
		b = in_samp->b2;
		r = in_samp->b3;
		break;
	case RAW888_RGB:
		r = in_samp->b1;
		g = in_samp->b2;
		b = in_samp->b3;
		break;
	case RAW888_BGR:
		b = in_samp->b1;
		g = in_samp->b2;
		r = in_samp->b3;
		break;
	case RAW888_RBG:
		r = in_samp->b1;
		b = in_samp->b2;
		g = in_samp->b3;
		break;
	case RAW888_BRG:
		b = in_samp->b1;
		r = in_samp->b2;
		g = in_samp->b3;
		break;
	default:
		;
	};

	out_samp->b1 = r;
	out_samp->b2 = g;
	out_samp->b3 = b;
}

void rgb565_normalization(unsigned int fmt, struct rgb565_sample *in_samp,
			  struct rgb565_sample *out_samp)
{
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;

	if (!DF_IS_RGB565(fmt))
		return;

	switch(fmt & RAW565_MASK) {
	case RAW565_GRB:
		g = in_samp->b1;
		r = in_samp->b2;
		b = in_samp->b3;
		break;
	case RAW565_GBR:
		g = in_samp->b1;
		b = in_samp->b2;
		r = in_samp->b3;
		break;
	case RAW565_RGB:
		r = in_samp->b1;
		g = in_samp->b2;
		b = in_samp->b3;
		break;
	case RAW565_BGR:
		b = in_samp->b1;
		g = in_samp->b2;
		r = in_samp->b3;
		break;
	case RAW565_RBG:
		r = in_samp->b1;
		b = in_samp->b2;
		g = in_samp->b3;
		break;
	case RAW565_BRG:
		b = in_samp->b1;
		r = in_samp->b2;
		g = in_samp->b3;
		break;
	default:
		;
	};

	out_samp->b1 = r;
	out_samp->b2 = g;
	out_samp->b3 = b;
}
void yCbCr422_pack_to_planar(unsigned char *y, unsigned char *cb, unsigned char *cr, const unsigned char *src, struct display_info *dis)
{
	int width = dis->width;
	int height = dis->height;
	int fmt = dis->fmt.format;
	struct yuv422_sample yuv422_samp;
	int line, col;
	int y_index = 0;
	int uv_index = 0;
	struct yuv422_sample *src_samp = (struct yuv422_sample *)src;

	for (line = 0; line < height; line++) {
		for (col = 0; col < width; col += 2) {
			yCbCr422_normalization(fmt, src_samp, &yuv422_samp);
			y[y_index++] = yuv422_samp.b1;
			cb[uv_index] = yuv422_samp.b2;
			y[y_index++] = yuv422_samp.b3;
			cr[uv_index] = yuv422_samp.b4;

			uv_index++;
			src_samp++;
		}
	}
}

