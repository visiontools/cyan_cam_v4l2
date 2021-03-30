
#include <string.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

void stop_device(cam_v4l2_t *cam) {
    enum v4l2_buf_type type;

    switch (cam->io) {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;
        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (-1 == xioctl(cam->fd, VIDIOC_STREAMOFF, &type)) {
                    fprintf(stderr, "VIDIOC_STREAMOFF\n");
                    exit(EXIT_FAILURE);
            }
            break;
    }
}
