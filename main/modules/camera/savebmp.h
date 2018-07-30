#ifndef __SAVE_BMP_H_
#define __SAVE_BMP_H_
#include "disp_info.h"
typedef unsigned short	WORD;	//2 Bytes
typedef unsigned long	DWORD;	//4 Bytes

#pragma pack(push)
#pragma pack(2)
typedef struct
{
	unsigned short	bfType;		/* "BM"(0x4D42) */
	unsigned long	bfSize;		/*  */
	unsigned short	bfReserved1;	/* */
	unsigned short	bfReserved2;	/* */
	unsigned long	bfoffBits;
} BITMAPFILEHEADER;

typedef struct
{
	unsigned long   biSize;		/*   size   of   BITMAPINFOHEADER   */
	unsigned long   biWidth;	/*   Î»Í¼¿í¶È(ÏñËØ)   */
	unsigned long   biHeight;	/*   Î»Í¼¸ß¶È(ÏñËØ)   */
	unsigned short  biPlanes;	/*   Ä¿±êÉè±¸µÄÎ»Æ½ÃæÊý,   ±ØÐëÖÃÎª1   */
	unsigned short  biBitCount;	/*   Ã¿¸öÏñËØµÄÎ»Êý,   1,4,8»ò24   */
	unsigned long   biCompress;	/*   Î»Í¼ÕóÁÐµÄÑ¹Ëõ·½·¨,0=²»Ñ¹Ëõ   */
	unsigned long   biSizeImage;	/*   Í¼Ïñ´óÐ¡(×Ö½Ú)   */
	unsigned long   biXPelsPerMeter;	/*   Ä¿±êÉè±¸Ë®Æ½Ã¿Ã×ÏñËØ¸öÊý   */
	unsigned long   biYPelsPerMeter;	/*   Ä¿±êÉè±¸´¹Ö±Ã¿Ã×ÏñËØ¸öÊý   */
	unsigned long   biClrUsed;		/*   Î»Í¼Êµ¼ÊÊ¹ÓÃµÄÑÕÉ«±íµÄÑÕÉ«Êý   */
	unsigned long   biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

struct bmp_bgr {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
};

extern int convert_yuv_to_rgb24(unsigned char *frame, unsigned char *rgb, struct display_info *dis);
extern int save_bgr_to_bmp(unsigned char *buf, struct display_info *dis, FILE *fp);
#endif /* __SAVE_BMP_H_ */
