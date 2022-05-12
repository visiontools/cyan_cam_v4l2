#include "cam_v4l2.h"

int
start_acqui(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;
	device_start(camera);
	return ERR_OK;
}

