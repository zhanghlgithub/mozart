#ifndef __DISPLAY_INFO_H__
#define __DISPLAY_INFO_H__

enum options {
	OPS_PREVIEW             = 0,
	OPS_SAVE_BMP            = 1,
	OPS_SAVE_JPG            = 2,
	OPS_SAVE_RAW            = 3,
	OPS_DISPLAY_RAW         = 4,
};

struct display_info {
	// format & resolution
	struct{
		unsigned int fourcc;   // compatible with V4L2
		unsigned int format;   // local definition, more detail
		unsigned int fmt_priv; // CIMCFG.PACK & CIMCFG.SEP && CIMCR.MBEN
	} fmt;
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	
	unsigned int xoff;
	unsigned int yoff;

	enum options ops;

	unsigned char *ybuf;
	unsigned char *ubuf;
	unsigned char *vbuf;
	unsigned int  n_bytes;

	unsigned int   only_y  : 1,    //only Y(ingnore UV)
		       window  : 1,    //window cut or not
		       userptr : 1,    //use internal TLB (using user space virtual memory instead of kernel space physical memory)
		       reserved: 29;
	unsigned int padding[2];
};
#endif


