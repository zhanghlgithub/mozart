#include <stdio.h>
#include "savebmp.h"
#include "disp_info.h"
#include "pixel.h"
#include "camera.h"
#include "convert.h"
BITMAPFILEHEADER  bfile = {
	.bfType = 0x4d42,	//"BM"
	.bfSize = 800*600*3+0x36,	//frame size + headers
	.bfReserved1 = 0,	
	.bfReserved2 = 0,	
	.bfoffBits = 0x36
};

BITMAPINFOHEADER  binfo = {
	.biSize = 0x28,		//sizeof(BITMAPINFOHEADER)
	.biWidth = 800,
	.biHeight = 600,
	.biPlanes = 1,
	.biBitCount = 0x18,	//24-bit true color
	.biCompress = 0,	//no compress
	.biSizeImage = 0,	//?
	.biXPelsPerMeter = 0x0b40,	//?
	.biYPelsPerMeter = 0x0b40,	//?
	.biClrUsed = 0,		//?
	.biClrImportant = 0	//?
};

int save_bgr_to_bmp(unsigned char *buf, struct display_info *dis, FILE *fp)
{
	int len = 0;
	int ret = 0;

	bfile.bfSize = dis->width * dis->height * 3;
	binfo.biWidth = dis->width;
	binfo.biHeight = dis->height;

	len = bfile.bfSize;

	binfo.biXPelsPerMeter = 0;
	binfo.biYPelsPerMeter = 0;

	ret = fwrite(&bfile, sizeof(BITMAPFILEHEADER), 1, fp);
	if (ret != 1) {
		printf("%s:%d write error!!!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = fwrite(&binfo, sizeof(BITMAPINFOHEADER), 1, fp);
	if (ret != 1) {
		printf("%s:%d write error!!!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = fwrite(buf, sizeof(unsigned char), len, fp);
	if (ret != len) {
		printf("%s:%d write error!!!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

int convert_yuv_to_rgb24(unsigned char *frame, unsigned char *rgb, struct display_info *dis)
{
	int fourcc = dis->fmt.fourcc;
	int format = dis->fmt.format;

	int line, col;
	int colorR, colorG, colorB;
	struct yuv422_sample yuv422_samp;
	struct yuv422_sample *yuv422_pixel =  NULL;

	unsigned char y0 = 0;
	unsigned char y1 = 0;
	unsigned char u = 0;
	unsigned char v = 0;
       

	struct bmp_bgr *bgr = (struct bmp_bgr *)rgb;
	dis->yoff = 0;
	dis->xoff = 0;
	for (line = dis->yoff; (line < (dis->yoff + dis->height)); line++)
	{
		bgr = (struct bmp_bgr *)(rgb + dis->width * (dis->height - line - 1) * 3);

		if (DF_IS_YCbCr422(fourcc)) {
			yuv422_pixel = (struct yuv422_sample *)
				(frame + line * dis->width * 2 + dis->xoff * 2);
		} else
			printf("error!!! the format is not support!!!\n");
		for (col = dis->xoff; (col < (dis->xoff + dis->width)); col += 2)
		{
			if (DF_IS_YCbCr422(fourcc)) {
				yCbCr422_normalization(format, yuv422_pixel, &yuv422_samp);
				y0 = yuv422_samp.b1;
				u = yuv422_samp.b2;
				y1 = yuv422_samp.b3;
				v = yuv422_samp.b4;

				yuv422_pixel ++;
			} else 
				printf("error,%s L%d:the format is not support!!!\n",__func__,__LINE__);
			if (dis->only_y) {
				fprintf(stdout, "-->%s L%d: ignore uv.\n", __func__, __LINE__);
				u = 128;
				v = 128;
			}

			//convert from YUV domain to RGB domain
			colorB = y0 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y0 -((179 * (v - 128) + 86 * (u - 128)) >> 8);
			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y0 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			//Windows BitMap BGR
			bgr->blue = colorB;
			bgr->green = colorG;
			bgr->red = colorR;
			bgr++;

			colorB = y1 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y1 -((179 * (v - 128) + 86 * (u - 128)) >> 8);
			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y1 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			//Windows BitMap BGR
			bgr->blue = colorB;
			bgr->green = colorG;
			bgr->red = colorR;
			bgr++;
		}
	}
	return 0;
}

