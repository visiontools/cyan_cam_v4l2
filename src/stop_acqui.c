#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

int
stop_acqui(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;

	enum v4l2_buf_type type;

	switch (camera->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(camera->fd, VIDIOC_STREAMOFF, &type)) {
			fprintf(stderr, "VIDIOC_STREAMOFF\n");
			return ERR_NOPE;
		}
		break;
	}
	return ERR_OK;
}
