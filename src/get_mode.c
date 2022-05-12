#include "cam_v4l2.h"

int
get_mode(void *cam_handle, int *mode)
{
	cam_v4l2_t *camera = cam_handle;
	if (camera->current_mode <0) {
        fprintf(stderr,"[cam_v4l2] Mode error. \n" ) ;
        return ERR_NOPE;
	}
	*mode = camera->current_mode;
	return ERR_OK;
}
