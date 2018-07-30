#ifndef __CIM_VIDEO_H__
#define __CIM_VIDEO_H__
#include  "disp_info.h"
extern int open_device(const char * dev_name);
extern int cim_init_disp_info(struct display_info *disp_info);
extern int init_device(struct display_info *disp);
extern void uninit_device(void);
extern void frame_handler_init(void *handle);
extern void start_capturing();
extern void stop_capturing();
extern void cim_read_frame();
#endif
