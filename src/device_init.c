#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "cam_v4l2.h"

void init_read( cam_v4l2_t* cam, unsigned int buffer_size) ;
void init_userp(cam_v4l2_t *cam, unsigned int buffer_size) ;
void init_mmap(cam_v4l2_t* cam) ;

void device_init(cam_v4l2_t* cam) {

    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    
    unsigned int min;

    if (-1 == xioctl(cam->fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\\n",
                    cam->dev_name);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, "VIDIOC_QUERYCAP error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\\n",
                cam->dev_name);
        exit(EXIT_FAILURE);
    }

    switch (cam->io) {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                fprintf(stderr, "%s does not support read i/o\\n",
                        cam->dev_name);
                exit(EXIT_FAILURE);
            }
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\\n",
                        cam->dev_name);
                exit(EXIT_FAILURE);
            }
            break;
    }


    /* Select video input, video standard and tune here. */


    memset(&cropcap, 0, sizeof(cropcap)) ;

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(cam->fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(cam->fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    memset(&fmt, 0, sizeof(fmt)) ;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /* Preserve original settings as set by v4l2-ctl for example */
    if (-1 == xioctl(cam->fd, VIDIOC_G_FMT, &fmt)) {
        fprintf(stderr, "VIDIOC_G_FMT error %d, %s\\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (cam->io) {
        case IO_METHOD_READ:
            init_read(cam,fmt.fmt.pix.sizeimage);
            break;

        case IO_METHOD_MMAP:
            init_mmap(cam);
            break;

        case IO_METHOD_USERPTR:
            init_userp(cam, fmt.fmt.pix.sizeimage);
            break;
    }
}


void init_read(cam_v4l2_t *cam, unsigned int buffer_size)
{
    cam->buffers = calloc(1, sizeof(*(cam->buffers)));

    if (!cam->buffers) {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }

    cam->buffers[0].length = buffer_size;
    cam->buffers[0].start = malloc(buffer_size);

    if (!cam->buffers[0].start) {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }
}

void init_mmap(cam_v4l2_t* cam)
{
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req)) ;

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                    "memory mappingn", cam->dev_name);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, "VIDIOC_REQBUFS error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\\n",
                cam->dev_name);
        exit(EXIT_FAILURE);
    }

    cam->buffers = calloc(req.count, sizeof(*(cam->buffers)));

    if (!cam->buffers) {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }

    for (cam->n_buffers = 0; cam->n_buffers < req.count; ++(cam->n_buffers)) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf)) ;

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = cam->n_buffers;

        if (-1 == xioctl(cam->fd, VIDIOC_QUERYBUF, &buf)){
            fprintf(stderr, "VIDIOC_QUERYBUF error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        cam->buffers[cam->n_buffers].length = buf.length;
        cam->buffers[cam->n_buffers].start =
            mmap(NULL /* start anywhere */,
                    buf.length,
                    PROT_READ | PROT_WRITE /* required */,
                    MAP_SHARED /* recommended */,
                    cam->fd, buf.m.offset);

        if (MAP_FAILED == cam->buffers[cam->n_buffers].start){
            fprintf(stderr, "mmap error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void init_userp(cam_v4l2_t *cam, unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req)) ;

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                    "user pointer i/on", cam->dev_name);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, "VIDIOC_REQBUFS error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    cam->buffers = calloc(4, sizeof(*(cam->buffers)));

    if (!cam->buffers) {
        fprintf(stderr, "Out of memory\\n");
        exit(EXIT_FAILURE);
    }

    for (cam->n_buffers = 0; cam->n_buffers < 4; ++(cam->n_buffers)) {
        cam->buffers[cam->n_buffers].length = buffer_size;
        cam->buffers[cam->n_buffers].start = malloc(buffer_size);

        if (!cam->buffers[cam->n_buffers].start) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
        }
    }
}

