#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

static void process_image(const void *p, int size)
{
    fflush(stderr);
    fprintf(stderr, ".");
    fflush(stdout);
}

static int read_frame(cam_v4l2_t *cam)
{
    struct v4l2_buffer buf;
    unsigned int i;

    switch (cam->io) {
        case IO_METHOD_READ:
            if (-1 == read(cam->fd, cam->buffers[0].start, cam->buffers[0].length)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;
                    case EIO:
                        /* Could ignore EIO, see spec. */
                        /* fall through */
                    default:
                        fprintf(stderr, "read\n");
                        exit(EXIT_FAILURE);
                }
            }
            process_image(cam->buffers[0].start, cam->buffers[0].length);
            break;
        case IO_METHOD_MMAP:
            memset(&buf, 0, sizeof(buf)) ;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if (-1 == xioctl(cam->fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        fprintf(stderr, "VIDIOC_DQBUF\n");
                        exit(EXIT_FAILURE);
                }
            }
            assert(buf.index < cam->n_buffers);
            process_image(cam->buffers[buf.index].start, buf.bytesused);
            if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf)){
                fprintf(stderr, "VIDIOC_QBUF\n");
                exit(EXIT_FAILURE);
            }
            break;
        case IO_METHOD_USERPTR:
            memset( &buf, 0, sizeof(buf) ) ;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            if (-1 == xioctl(cam->fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;
                    case EIO:
                        /* Could ignore EIO, see spec. */
                        /* fall through */
                    default:
                        fprintf(stderr, "VIDIOC_DQBUF\n");
                        exit(EXIT_FAILURE);
                }
            }
            for (i = 0; i < cam->n_buffers; ++i)
                if (buf.m.userptr == (unsigned long)cam->buffers[i].start
                        && buf.length == cam->buffers[i].length)
                    break;
            assert(i < cam->n_buffers);
            process_image((void *)buf.m.userptr, buf.bytesused);
            if (-1 == xioctl(cam->fd, VIDIOC_QBUF, &buf)){
                fprintf(stderr, "VIDIOC_QBUF\n");
                exit(EXIT_FAILURE);
            }
            break;
    }
    return 1;
}

void device_get_frame(cam_v4l2_t *cam) {

            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(cam->fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(cam->fd + 1, &fds, NULL, NULL, &tv);
            if (-1 == r) {
                if (EINTR == errno) {
                    fprintf(stderr,"Signal caught\n") ;
                    return ;
                }
                fprintf(stderr, "select\n");
                exit(EXIT_FAILURE);
            }

            if (0 == r) {
                fprintf(stderr, "select timeout\\n");
                exit(EXIT_FAILURE);
            }

            read_frame(cam) ;
              
}

