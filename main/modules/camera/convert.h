#ifndef __CIM_CONVERT_H__
#define __CIM_CONVERT_H__

extern void yCbCr422_normalization(unsigned int fmt, struct yuv422_sample *yuv422_samp_in,
		                          struct yuv422_sample *yuv422_samp_out);
#endif
