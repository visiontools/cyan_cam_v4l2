#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

int buffers_init_read(cam_v4l2_t * cam, unsigned int buffer_size);
int buffers_init_userp(cam_v4l2_t * cam, unsigned int buffer_size);
int buffers_init_mmap(cam_v4l2_t * cam);

int
set_mode(void *cam_handle, int mode)
{
	cam_v4l2_t *camera = cam_handle;
	
    struct v4l2_format fmt;
	struct v4l2_streamparm parm;

    // Retrieve previous Image format

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
    if (-1 == xioctl(camera->fd, VIDIOC_G_FMT, &fmt)) {
		fprintf(stderr, "VIDIOC_G_FMT error %d, %s\\n", errno, strerror(errno));
		return ERR_NOPE;
	}

	// Set Image format

	fmt.fmt.pix.pixelformat = camera->v4l_modes[mode].v4l_format;
	fmt.fmt.pix.width = camera->v4l_modes[mode].v4l_width;
	fmt.fmt.pix.height = camera->v4l_modes[mode].v4l_height;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

    // Apply image format to camera

	xioctl(camera->fd, VIDIOC_S_FMT, &fmt);

	if (fmt.fmt.pix.pixelformat != camera->v4l_modes[mode].v4l_format) {
		fprintf(stderr, "[cam_v4l2] Libv4l didn't accept pix.pixelformat.\n");
		return ERR_NOPE;
	}
	if (fmt.fmt.pix.width != camera->v4l_modes[mode].v4l_width) {
		fprintf(stderr, "[cam_v4l2] Libv4l didn't accept pix.width\n");
		return ERR_NOPE;
	}
	if (fmt.fmt.pix.height != camera->v4l_modes[mode].v4l_height) {
		fprintf(stderr, "[cam_v4l2] Libv4l didn't accept pix.height\n");
		return ERR_NOPE;
	}
	if (fmt.fmt.pix.field != V4L2_FIELD_NONE) {
		fprintf(stderr, "[cam_v4l2] Libv4l didn't accept pix.field\n");
		fprintf(stderr, "[cam_v4l2] Current value is %d\n", fmt.fmt.pix.field);
		fprintf(stderr, "[cam_v4l2] Only progressive scan is supported\n");
		return ERR_NOPE;
	}

	// Set FPS

	memset(&parm, 0, sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	parm.parm.capture.timeperframe.numerator =
	    camera->v4l_modes[mode].v4l_fps_numerator;
	parm.parm.capture.timeperframe.denominator =
	    camera->v4l_modes[mode].v4l_fps_denominator;

	if (xioctl(camera->fd, VIDIOC_S_PARM, &parm) < 0) {
		fprintf(stderr, "[cam_v4l2] Libv4l didn't accept framerate.\n");
		return ERR_NOPE;
	}

	// Set pixel_decode function

	camera->buff_decode = camera->v4l_modes[mode].buff_decode ;

	// Set current_mode

	camera->current_mode = mode;

    // Prepare buffers for given IO method and Image format

#if VERBOSE
    printf("width :  %d \n", fmt.fmt.pix.width ) ;
    printf("height : %d \n", fmt.fmt.pix.height ) ;
    printf("Sizeimage: %d\n", ftm.ftm.pix.sizeimage ) ;
#endif

    switch (camera->io) {
	case IO_METHOD_READ:
		buffers_init_read(camera, fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		buffers_init_mmap(camera);
		break;

	case IO_METHOD_USERPTR:
		buffers_init_userp(camera, fmt.fmt.pix.sizeimage);
		break;
	}

	return ERR_OK;
}


