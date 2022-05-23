#include <stdlib.h>
#include <stdio.h>
#include "cam_v4l2.h"

int
buffers_init_read(cam_v4l2_t * cam, unsigned int buffer_size)
{
	cam->buffers = calloc(1, sizeof(*(cam->buffers)));

	if (!cam->buffers) {
		fprintf(stderr, "Out of memory\\n");
		return ERR_NOPE;
	}

	cam->buffers[0].length = buffer_size;
	cam->buffers[0].start = malloc(buffer_size);

	if (!cam->buffers[0].start) {
		fprintf(stderr, "Out of memory\\n");
		return ERR_NOPE;
	}
    return ERR_OK;
}

