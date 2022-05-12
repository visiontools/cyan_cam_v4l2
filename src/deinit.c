#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cam_v4l2.h"

int
deinit(void *cam_handle)
{
	unsigned int i;
	
    cam_v4l2_t *camera = cam_handle;

    // Deinit

	switch (camera->io) {
	case IO_METHOD_READ:
		free(camera->buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < camera->n_buffers; ++i)
			if (-1 == munmap(camera->buffers[i].start,
				camera->buffers[i].length)) {
				fprintf(stderr, "munmap error %d, %s\\n",
				    errno, strerror(errno));
				exit(EXIT_FAILURE);
			}
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < camera->n_buffers; ++i)
			free(camera->buffers[i].start);
		break;
	}

	free(camera->buffers);
	free(camera->modes);
	free(camera->v4l_modes);

	camera->modes = NULL;
	camera->v4l_modes = NULL;
	camera->nb_modes = 0;
	camera->current_mode = -1;
	camera->n_buffers = 0;
	camera->buffers = NULL;


    // Close device


	if (-1 == close(camera->fd)) {
		fprintf(stderr, "close error %d, %s\\n", errno,
		    strerror(errno));
		exit(EXIT_FAILURE);
	}
	camera->fd = -1;

	free(cam_handle);
	return ERR_OK;
}
