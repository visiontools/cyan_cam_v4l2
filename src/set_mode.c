#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

int
set_mode(void *cam_handle, int mode)
{
	cam_v4l2_t *camera = cam_handle;
	struct v4l2_format fmt;
	struct v4l2_streamparm parm;

	// Set Image mode

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = camera->v4l_modes[mode].v4l_width;
	fmt.fmt.pix.height = camera->v4l_modes[mode].v4l_height;
	fmt.fmt.pix.pixelformat = camera->v4l_modes[mode].v4l_format;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	xioctl(camera->fd, VIDIOC_S_FMT, &fmt);
	if (fmt.fmt.pix.pixelformat != camera->v4l_modes[mode].v4l_format) {
		fprintf(stderr, "[cam_v4l] Libv4l didn't accept format.\n");
		return ERR_NOPE;
	}
	// Set FPS

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator =
	    camera->v4l_modes[mode].v4l_fps_numerator;
	parm.parm.capture.timeperframe.denominator =
	    camera->v4l_modes[mode].v4l_fps_denominator;

	if (xioctl(camera->fd, VIDIOC_S_PARM, &parm) < 0) {
		fprintf(stderr, "[cam_v4l] Libv4l didn't accept framerate.\n");
		return ERR_NOPE;
	}
	// Set pixel_decode function

	camera->buff_decode = camera->v4l_modes[mode].buff_decode ;

	// Set current_mode

	camera->current_mode = mode;

	return ERR_OK;
}
