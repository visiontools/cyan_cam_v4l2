#include "cam_v4l2.h"

int
get_available_modes(void *cam_handle, hw_mode_t ** modes, int *nb_modes)
{
	cam_v4l2_t *camera = cam_handle;
	if (camera->modes == NULL)
		return ERR_NOPE;
	if (camera->nb_modes == 0)
		return ERR_NOPE;
	*modes = camera->modes;
	*nb_modes = camera->nb_modes;
	return ERR_OK;
}
