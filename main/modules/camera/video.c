/*
 * V4L2 Spec Appendix B.
 *
 * Video Capture Example
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>	//getopt_long()

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <asm/types.h>	//for videodev2.h

#include <linux/videodev2.h>
#include "disp_info.h"
#include "camera.h"
#include "jz_cim.h"

#define CLEAR(x)	memset(&(x), 0, sizeof(x))

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
} io_method;

static char *io_method_str[2] = {
	"IO_METHOD_READ",
	"IO_METHOD_MMAP",
};

unsigned int  cim_yuv422_formats[8] = {
	YCbCr422_CrY1CbY0,      //CIMCFG.PACK[6:4]=0
	YCbCr422_Y0CrY1Cb,      //CIMCFG.PACK[6:4]=1
	YCbCr422_CbY0CrY1,      //CIMCFG.PACK[6:4]=2
	YCbCr422_Y1CbY0Cr,      //CIMCFG.PACK[6:4]=3
	YCbCr422_Y0CbY1Cr,      //CIMCFG.PACK[6:4]=4
	YCbCr422_CrY0CbY1,      //CIMCFG.PACK[6:4]=5
	YCbCr422_Y1CrY0Cb,      //CIMCFG.PACK[6:4]=6
	YCbCr422_CbY1CrY0,      //CIMCFG.PACK[6:4]=7
};

struct buffer {
	void *	start;
	size_t	length;
};
static int cim_stop_flag = 0;
static char cim_name[16] = {0};
static int fd;
static io_method	io		= IO_METHOD_MMAP;
struct buffer *		buffers		= NULL;
static unsigned int	n_buffers	= 0;

typedef  int (*hand_t)(void *buf);
static hand_t frame_handler;

static int errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	return -1;
}

static int xioctl(int fd,int request, void *arg)
{
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static int read_frame()
{
	struct v4l2_buffer buf;
	switch (io) {
	case IO_METHOD_READ:
		printf("===>IO_METHOD_READ\n");
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("read");
			}
		}
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < n_buffers);
                if(frame_handler) {
	                    frame_handler((void *)buffers[buf.index].start);
		}

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;

	}

	return 1;
}

void cim_read_frame()
{
//	for (;;) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* Timeout. */
		tv.tv_sec = 20;
		tv.tv_usec = 0;

		r = select(fd+1, &fds, NULL, NULL, &tv);
		if (r == -1) {
		//	if (EINTR == errno)
		//		continue;
			errno_exit("select");
		}
		if (r == 0) {
			fprintf(stderr, "select timeout\n");
			return ;
		}
		read_frame();
		//if (read_frame() == 1)
		//	break;
//	}
}

void stop_capturing()
{
	enum v4l2_buf_type type;
	cim_stop_flag = 1;
	switch (io) {
	case IO_METHOD_READ:
		/* nothing to do */
		break;
	case IO_METHOD_MMAP:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");

		break;
	}
}

void start_capturing()
{
	unsigned int i;
	enum v4l2_buf_type type;
	cim_stop_flag = 0;
	switch(io) {
	case IO_METHOD_READ:
		/* nothing to do */
		break;
	case IO_METHOD_MMAP:
		for (i=0; i<n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;
	}
}


static void init_read(unsigned int buffer_size)
{
	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return ;
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory2\n");
		return;
	}
}

static void init_mmap()
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count	= 1;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support memory mapping\n", cim_name);
			return ;
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 1) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", cim_name);
		return ;
	}

	buffers = malloc(req.count * sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return ;
	}

	for (n_buffers=0; n_buffers<req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

#if 0
static int init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size = 0;
	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count	= 1;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		fprintf(stderr,"VIDIOC_REQBUFS error\n");
		return -1;
	}

	buffers = calloc(req.count, sizeof(*buffers));
	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return -1;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = (unsigned char *)JZMalloc(128, buffer_size);
		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			return -1;
		}
	}
	return 0;
}

#endif
int open_device(const char * dev_name)
{
	struct stat st;
	strcpy(cim_name,dev_name);
	if (-1 == stat(cim_name, &st)) {
		fprintf(stderr, "Cannot identify '%s':%d, %s\n", cim_name, errno, strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is not char device\n", cim_name);
		return -1;
	}

	fd = open(cim_name, O_RDWR | O_NONBLOCK, 0);
	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s':%d, %s\n", cim_name, errno, strerror(errno));
		return -1;
	}
	return fd;
}
#if 0
static void query_cap(struct v4l2_capability *cap)
{
	if (cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)
		fprintf(stdout, "Video capture device\n");
	if (cap->capabilities & V4L2_CAP_READWRITE)
		fprintf(stdout, "Read/Write systemcalls\n");
	if (cap->capabilities & V4L2_CAP_STREAMING)
		fprintf(stdout, "Streaming I/O ioctls\n");
}
static void dump_fmt(struct v4l2_format *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		fprintf(stdout, "width=%d, height=%d\n", fmt->fmt.pix.width, fmt->fmt.pix.height);
		fprintf(stdout, "pixelformat=%s, field=%d\n", (char *)&fmt->fmt.pix.pixelformat, fmt->fmt.pix.field);
		fprintf(stdout, "bytesperline=%d, sizeimage=%d\n", fmt->fmt.pix.bytesperline, fmt->fmt.pix.sizeimage);
		fprintf(stdout, "colorspace=%d\n", fmt->fmt.pix.colorspace);
	}
}

#endif
unsigned int set_format(struct display_info *disp)
{
	struct v4l2_format fmt;
	unsigned int min;
	CLEAR(fmt);
	fmt.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width	= disp->width;
	fmt.fmt.pix.height	= disp->height;
	fmt.fmt.pix.pixelformat	= disp->fmt.fourcc;
	fmt.fmt.pix.field	= V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;
	fmt.fmt.pix.priv	= disp->fmt.fmt_priv;


	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
	{
	        printf("==>%s L%d: set error  !!!!!!!!!!!!! \n",__func__, __LINE__);
		errno_exit("VIDIOC_S_FMT");
        }

        xioctl(fd, VIDIOC_G_FMT, &fmt);
	if((disp->width != fmt.fmt.pix.width)||(disp->height != fmt.fmt.pix.height)){
                disp->width = fmt.fmt.pix.width;
		disp->height = fmt.fmt.pix.height;
	}

	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	printf("==>%s L%d: sizeimage=0x%x fmt.fmt.pix.width = 0x%x fmt.fmt.pix.height = 0x%x\n", __func__, __LINE__, fmt.fmt.pix.sizeimage,fmt.fmt.pix.width, fmt.fmt.pix.height);

	return fmt.fmt.pix.sizeimage;
}

int cim_init_disp_info(struct display_info *disp_info)
{
	int packing = 4;        /* CIMCFG.PACK */
	char sep_mb = 0;
	switch (disp_info->fmt.format) {
		case DISP_FMT_YCbCr422:
			disp_info->fmt.format = cim_yuv422_formats[packing];
			disp_info->fmt.fourcc = V4L2_PIX_FMT_YUYV;
			break;
		case DISP_FMT_ITU656P:
			disp_info->fmt.format = ITU656P_YCbCr422_CbY0CrY1;       //itu656 progressive 8-bit ycbcr422
			disp_info->fmt.fourcc = V4L2_PIX_FMT_YUYV;
			break;
		case DISP_FMT_ITU656I:
			disp_info->fmt.format = ITU656I_YCbCr422_CbY0CrY1;       //itu656 interlace 8-bit ycbcr422
			disp_info->fmt.fourcc = V4L2_PIX_FMT_YUYV;
			break;
		default:
			disp_info->fmt.format = cim_yuv422_formats[packing];
			disp_info->fmt.fourcc = V4L2_PIX_FMT_YUYV;
			break;
	}

	sep_mb = V4L2_FRM_FMT_PACK;     //CIMCFG.SEP=0
	disp_info->fmt.fmt_priv = cim_set_fmt_priv(0, 0, packing, sep_mb);
	return 0;
}

int init_device(struct display_info *disp)

{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	unsigned int frmsize;
	
	io = IO_METHOD_MMAP;		// map device memory to userspace
	printf("==>%s L%d: io method: %s\n", __func__, __LINE__, io_method_str[io]);
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is not capture device\n", cim_name);
		return -1;
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s doesn't support read i/o\n", cim_name);
			return -1;
		}
		break;
	case IO_METHOD_MMAP:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s doesn't support streaming i/o\n", cim_name);
			return -1;
		}
		break;
	}

	/* select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	/* reset to default */
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch(errno) {
			case EINVAL:
				/* Cropping not supported */
				break;
			default:
				/* Errors ignored */
				break;
			}
		}
	}

	frmsize = set_format(disp);

	switch (io) {
	case IO_METHOD_READ:
		init_read(frmsize);
		break;
	case IO_METHOD_MMAP:
		init_mmap();
		break;
	}
	return 0;
}

void uninit_device(void)
{
	unsigned int i;

	switch(io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		for (i=0; i<n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	}

	free(buffers);
}

void frame_handler_init(void *handle)
{
        frame_handler = handle;
}
