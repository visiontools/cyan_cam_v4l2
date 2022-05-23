#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "cam_v4l2.h"

int
buffers_init_mmap(cam_v4l2_t * cam)
{
	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
			    "memory mappingn", cam->dev_name);
			return ERR_NOPE;
		} else {
			fprintf(stderr, "VIDIOC_REQBUFS error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\\n", cam->dev_name);
		return ERR_NOPE;
	}

	cam->buffers = calloc(req.count, sizeof(*(cam->buffers)));

	if (!cam->buffers) {
		fprintf(stderr, "Out of memory\\n");
		return ERR_NOPE;
	}

	for (cam->n_buffers = 0; cam->n_buffers < req.count;
	    ++(cam->n_buffers)) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = cam->n_buffers;

		if (-1 == xioctl(cam->fd, VIDIOC_QUERYBUF, &buf)) {
			fprintf(stderr, "VIDIOC_QUERYBUF error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}

		cam->buffers[cam->n_buffers].length = buf.length;
		cam->buffers[cam->n_buffers].start =
		    mmap(NULL /* start anywhere */ ,
		    buf.length, PROT_READ | PROT_WRITE /* required */ ,
		    MAP_SHARED /* recommended */ ,
		    cam->fd, buf.m.offset);

		if (MAP_FAILED == cam->buffers[cam->n_buffers].start) {
			fprintf(stderr, "mmap error %d, %s\\n", errno,
			    strerror(errno));
			return ERR_NOPE;
		}
	}
    return ERR_OK ;
}

