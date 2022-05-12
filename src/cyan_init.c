#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cam_v4l2.h"

int
init(void **cam_handle, va_list args)
{

	char *filename;
	cam_v4l2_t *camera;
	int i;

	filename = va_arg(args, char *);

#ifdef VERBOSE
	printf("filename: %s \n", filename);
#endif

	camera = (cam_v4l2_t *) malloc(sizeof(cam_v4l2_t));
	*cam_handle = (void *) camera;
	if (camera == NULL) {
		CYAN_ERROR(ERR_MALLOC);
		return ERR_MALLOC;
	}

	camera->dev_name = NULL;
	camera->fd = -1;
	camera->io = IO_METHOD_MMAP;
	camera->buffers = NULL;
	camera->n_buffers = 0;
	camera->v4l_modes = NULL;
	camera->modes = NULL;
	camera->nb_modes = 0;
	camera->current_mode = -1;

	camera->dev_name = malloc(255);
	strncpy(camera->dev_name, filename, 255);

	device_open(camera);
	device_init(camera);
	device_get_available_modes(camera);

#ifdef VERBOSE
	printf("Found %d acquisition modes \n", camera->nb_modes);
	for (i = 0; i < camera->nb_modes; i++) {
		printf("[%3d]%c %s\n", i,
                (camera->modes[i].enabled)?'+':'-',
                camera->modes[i].description);
	}
#endif

	for (i = 0; i < camera->nb_modes; i++) {
		if (camera->modes[i].enabled == 1) {
			if (set_mode(camera, i) == ERR_OK)
				break;
		}
	}

    if ( camera->current_mode == -1 ) {
        fprintf(stderr, "[cam_v4l2] Could not find any compatible mode. Exiting. \n");
        return ERR_NOPE ;
    }

#ifdef VERBOSE
	printf("Current Mode is %d \n", camera->current_mode);
#endif

	return ERR_OK;
}

