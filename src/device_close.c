#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "cam_v4l2.h"

void
device_close(cam_v4l2_t * cam)
{
	if (-1 == close(cam->fd)) {
		fprintf(stderr, "close error %d, %s\\n", errno,
		    strerror(errno));
		exit(EXIT_FAILURE);
	}
	cam->fd = -1;
}
