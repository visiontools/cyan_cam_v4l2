#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

void
device_start(cam_v4l2_t * cam)
{

	unsigned int i;
	enum v4l2_buf_type type;

	switch (cam->io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < cam->n_buffers; ++i) {
			struct v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf)) {
				fprintf(stderr, "VIDIOC_QBUF\n");
				exit(EXIT_FAILURE);
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(cam->fd, VIDIOC_STREAMON, &type)) {
			fprintf(stderr, "VIDIOC_STREAMON\n");
			exit(EXIT_FAILURE);
		}
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < cam->n_buffers; ++i) {
			struct v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) cam->buffers[i].start;
			buf.length = cam->buffers[i].length;

			if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf)) {
				fprintf(stderr, "VIDIOC_QBUF\n");
				exit(EXIT_FAILURE);
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(cam->fd, VIDIOC_STREAMON, &type)) {
			fprintf(stderr, "VIDIOC_STREAMON\n");
			exit(EXIT_FAILURE);
		}
		break;
	}
}
