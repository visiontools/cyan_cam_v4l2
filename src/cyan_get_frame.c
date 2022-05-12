#include "cam_v4l2.h"

int
get_frame(void *cam_handle, image_t * img)
{
	cam_v4l2_t *camera = cam_handle;
	device_get_frame(camera);
	// TODO
	return ERR_OK;
}
