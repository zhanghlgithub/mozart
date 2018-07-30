#include <sys/time.h>
#include <getopt.h>
#include <linux/fb.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include "camera/lcd.h"
#include "camera/camera.h"
#include "camera/disp_info.h"
#include "camera/savebmp.h"
#include "mozart_camera.h"
#include "tips_interface.h"
#define DEFAULT_WIDTH   320
#define DEFAULT_HEIGHT  240
#define CAMERA_NAME     "/dev/video0"
#define LCD_NAME        "/dev/fb0"

static int camera_stop_flag = 0;
static int take_picture =0;
static pthread_t read_thread;
static char filename[64];
static int count = 0;

pthread_mutex_t lock;
static struct fb_info *fb_info;
struct display_info disp_info;
static int fd = -1;
static int start_ms = 0;

void process_frame(const char *filename, void *frame, struct display_info *dis)
{
	 FILE *file;

	 int width = dis->width;
	 int height = dis->height;

	 unsigned char *rgb_frame = NULL;
	 file = fopen(filename, "w");
	 if (!file) {
		 fprintf(stderr, "--%s() L%d: open %s failed!!!\n", __func__, __LINE__, filename);
		 return;
	 }
	 rgb_frame = (unsigned char *)malloc(width * height * 3);        //rgb888 bpp=24
	 if (rgb_frame == NULL) {
		 fprintf(stderr, "-->%s() L%d: malloc failed!\n", __func__, __LINE__);
		 return;
	 }
	 convert_yuv_to_rgb24(frame, rgb_frame, dis);
	 save_bgr_to_bmp(rgb_frame, dis, file);
	 free(rgb_frame);
	 fclose(file);
}

static int handle_bufs(void *tmp_buf)
{
	struct timeval t_end;
	int end_ms;
	iq_lcd_display_frame(tmp_buf,&disp_info,fb_info);
	if(take_picture){
		sprintf(filename,"/mnt/sdcard/test%d.bmp",count);
		process_frame(filename, tmp_buf, &disp_info);
		gettimeofday(&t_end,NULL);
		end_ms = ((long long) t_end.tv_sec) * 1000 + (long long) t_end.tv_usec / 1000;
		if((end_ms - start_ms) > 500){
			mozart_play_key("take_picture");		//拍照的相机声
			count++;
			if(count > 200)
				count = 0;
			camera_stop_flag = 1;
		}
	}
	return 0;
}

static void * camera_read_frame(void *arg)
{
	while(!camera_stop_flag){
		pthread_mutex_lock(&lock);
		iq_camera_get_frames();
		pthread_mutex_unlock(&lock);
	}
	pthread_exit(NULL);
}

int mozart_start_camera()
{
	int width = DEFAULT_WIDTH;
	int height = DEFAULT_HEIGHT;
	int bpp = 16;
	int ret = -1;
	disp_info.width = width;
	disp_info.height = height;
	disp_info.bpp = bpp;
	take_picture = 0;
	if(fd >= 0)
		mozart_stop_camera();
	pthread_mutex_init(&lock, NULL);
	camera_stop_flag = 0;
	iq_camera_init_disp_info(&disp_info);
	fb_info = iq_lcd_init(LCD_NAME);
	fd = iq_camera_open(CAMERA_NAME);
	if(fd < 0){
		fprintf(stderr,"open camera failed!!!");
		return -1;
	}

	iq_camera_start(&disp_info,handle_bufs);
	ret = pthread_create(&read_thread,NULL,camera_read_frame,NULL);
	if(ret != 0){
		fprintf(stderr,"create pthread failed!!!\n");
		return -1;
	}
	
	return 0;

}
static void stop_camera()
{
	void *pret = NULL;
	pthread_join(read_thread,&pret);
	iq_camera_stop(&disp_info);
	iq_camera_close(fd);
	iq_lcd_deinit(fb_info);
	pthread_mutex_destroy(&lock);
	fd = -1;
}
void mozart_stop_camera()
{
	camera_stop_flag = 1;	
	stop_camera();
}

void mozart_take_picture()
{
	struct timeval t_start;
	take_picture  = 1;
	gettimeofday(&t_start,NULL);
	start_ms = ((long long) t_start.tv_sec) * 1000 + (long long) t_start.tv_usec / 1000;
	stop_camera();
	printf("finish %s\n",__func__);
}

