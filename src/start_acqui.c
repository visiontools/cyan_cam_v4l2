#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

int
start_acqui(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;

	unsigned int i;
	enum v4l2_buf_type type;

	switch (camera->io) {

	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < camera->n_buffers; ++i) {
			struct v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(camera->fd, VIDIOC_QBUF, &buf)) {
				fprintf(stderr, "VIDIOC_QBUF\n");
                return ERR_NOPE;
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(camera->fd, VIDIOC_STREAMON, &type)) {
			fprintf(stderr, "VIDIOC_STREAMON\n");
            return ERR_NOPE ;
		}
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < camera->n_buffers; ++i) {
			struct v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) camera->buffers[i].start;
			buf.length = camera->buffers[i].length;

			if (-1 == xioctl(camera->fd, VIDIOC_QBUF, &buf)) {
				fprintf(stderr, "VIDIOC_QBUF\n");
                return ERR_NOPE;
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(camera->fd, VIDIOC_STREAMON, &type)) {
			fprintf(stderr, "VIDIOC_STREAMON\n");
			return ERR_NOPE;
		}
		break;
	}

	return ERR_OK;
}
