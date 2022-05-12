#include <stdlib.h>
#include "cam_v4l2.h"

int
deinit(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;
	device_uninit(camera);
	device_close(camera);
	free(cam_handle);
	return ERR_OK;
}
