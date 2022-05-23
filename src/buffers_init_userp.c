#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

 
int
buffers_init_userp(cam_v4l2_t * cam, unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
			    "user pointer i/on", cam->dev_name);
			return ERR_NOPE;
		} else {
			fprintf(stderr, "VIDIOC_REQBUFS error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}
	}

	cam->buffers = calloc(4, sizeof(*(cam->buffers)));

	if (!cam->buffers) {
		fprintf(stderr, "Out of memory\\n");
		return ERR_NOPE;
	}

	for (cam->n_buffers = 0; cam->n_buffers < 4; ++(cam->n_buffers)) {
		cam->buffers[cam->n_buffers].length = buffer_size;
		cam->buffers[cam->n_buffers].start = malloc(buffer_size);

		if (!cam->buffers[cam->n_buffers].start) {
			fprintf(stderr, "Out of memory\\n");
			return ERR_NOPE;
		}
	}
    return ERR_OK;
}

