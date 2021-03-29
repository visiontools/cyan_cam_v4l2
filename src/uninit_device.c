#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "cam_v4l2.h"


void uninit_device(cam_v4l2_t *cam)
{
    unsigned int i;

    switch (cam->io) {
        case IO_METHOD_READ:
            free(cam->buffers[0].start);
            break;

        case IO_METHOD_MMAP:
            for (i = 0; i < cam->n_buffers; ++i)
                if (-1 == munmap(cam->buffers[i].start, cam->buffers[i].length)) {
                    fprintf(stderr, "munmap error %d, %s\\n", errno, strerror(errno));
                    exit(EXIT_FAILURE);
                }
            break;

        case IO_METHOD_USERPTR:
            for (i = 0; i < cam->n_buffers; ++i)
                free(cam->buffers[i].start);
            break;
    }

    free(cam->buffers);

    cam->n_buffers = 0 ;
    cam->buffers = NULL ;
}

