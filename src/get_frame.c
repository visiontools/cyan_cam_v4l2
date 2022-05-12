#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

static void
process_image(const void *p, int size)
{
	fflush(stderr);
	fprintf(stderr, ".");
	fflush(stdout);
}

int
get_frame(void *cam_handle, image_t * img)
{
	cam_v4l2_t *camera = cam_handle;

	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(camera->fd, &fds);

	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(camera->fd + 1, &fds, NULL, NULL, &tv);
	if (-1 == r) {
		if (EINTR == errno) {
			fprintf(stderr, "Signal caught\n");
			return ERR_NOPE;
		}
		fprintf(stderr, "select\n");
        return ERR_NOPE;
	}

	if (0 == r) {
		fprintf(stderr, "select timeout\\n");
		return ERR_NOPE;
	}

	struct v4l2_buffer buf;
	unsigned int i;

	switch (camera->io) {
	case IO_METHOD_READ:
		if (-1 == read(camera->fd, camera->buffers[0].start,
			camera->buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				fprintf(stderr, "read\n");
                return ERR_NOPE;
			}
		}
		process_image(camera->buffers[0].start, camera->buffers[0].length);
		break;
	case IO_METHOD_MMAP:
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (-1 == xioctl(camera->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				fprintf(stderr, "VIDIOC_DQBUF\n");
                return ERR_NOPE;
			}
		}
		assert(buf.index < camera->n_buffers);
		process_image(camera->buffers[buf.index].start, buf.bytesused);
		if (-1 == xioctl(camera->fd, VIDIOC_QBUF, &buf)) {
			fprintf(stderr, "VIDIOC_QBUF\n");
            return ERR_NOPE;
		}
		break;
	case IO_METHOD_USERPTR:
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		if (-1 == xioctl(camera->fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				fprintf(stderr, "VIDIOC_DQBUF\n");
                return ERR_NOPE;
			}
		}
		for (i = 0; i < camera->n_buffers; ++i)
			if (buf.m.userptr ==
			    (unsigned long) camera->buffers[i].start
			    && buf.length == camera->buffers[i].length)
				break;
		assert(i < camera->n_buffers);
		process_image((void *) buf.m.userptr, buf.bytesused);
		if (-1 == xioctl(camera->fd, VIDIOC_QBUF, &buf)) {
			fprintf(stderr, "VIDIOC_QBUF\n");
            return ERR_NOPE;
		}
		break;
	}
	return ERR_OK;
}
