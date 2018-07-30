#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include "disp_info.h"
#include "camera.h"
#include "jz_cim.h"
#include "video.h"
void iq_camera_close(int fd)
{
	if(fd >= 0)
		close(fd);
}
int iq_camera_open(const char * dev_name)
{
	int ret;
	ret = open_device(dev_name);
	return ret;
}
int iq_camera_init_disp_info(struct display_info *disp)
{
	cim_init_disp_info(disp);
	return 0;
}
int iq_camera_start(struct display_info *disp,void *handle)
{
	init_device(disp);
	start_capturing();
	frame_handler_init(handle);
	return 0;
}
int iq_camera_stop(struct display_info * disp_info)
{
	stop_capturing();
	uninit_device();
	if (disp_info->ybuf)
		free(disp_info->ybuf);
	if(disp_info->ubuf)
		free(disp_info->ubuf);
	if(disp_info->vbuf)	
		free(disp_info->vbuf);
	return 0;
}
int iq_camera_set_fps(int fps)
{
	return 0;
}
int iq_camera_set_resolution(int resolution)
{
	return 0;
}

int iq_camera_get_frames()
{
	cim_read_frame();
	return 0;
}
