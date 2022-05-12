#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <linux/videodev2.h>

#include "cam_v4l2.h"

#define VERBOSE

// Cyan interface

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

#ifdef VERBOSE
	printf("Current Mode is %d \n", camera->current_mode);
#endif

	return ERR_OK;
}

int
deinit(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;

	device_uninit(camera);
	device_close(camera);

	free(cam_handle);
	return ERR_OK;
}

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

int
get_serial(void *cam_handle, char **serial, size_t *serial_size)
{
	// TODO
	return ERR_NOPE;
}

int
set_mode(void *cam_handle, int mode)
{
	cam_v4l2_t *camera = cam_handle;
	struct v4l2_format fmt;
	struct v4l2_streamparm parm;

	// Set Image mode

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = camera->v4l_modes[mode].v4l_width;
	fmt.fmt.pix.height = camera->v4l_modes[mode].v4l_height;
	fmt.fmt.pix.pixelformat = camera->v4l_modes[mode].v4l_format;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	xioctl(camera->fd, VIDIOC_S_FMT, &fmt);
	if (fmt.fmt.pix.pixelformat != camera->v4l_modes[mode].v4l_format) {
		fprintf(stderr, "[cam_v4l] Libv4l didn't accept format.\n");
		return ERR_NOPE;
	}
	// Set FPS

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator =
	    camera->v4l_modes[mode].v4l_fps_numerator;
	parm.parm.capture.timeperframe.denominator =
	    camera->v4l_modes[mode].v4l_fps_denominator;

	if (xioctl(camera->fd, VIDIOC_S_PARM, &parm) < 0) {
		fprintf(stderr, "[cam_v4l] Libv4l didn't accept framerate.\n");
		return ERR_NOPE;
	}
	// Set pixel_decode function

	// TODO

	// Set current_mode

	camera->current_mode = mode;

	return ERR_OK;
}

int
get_mode(void *cam_handle, int *mode)
{
	cam_v4l2_t *camera = cam_handle;
	if (camera->current_mode == -1) {
		set_mode(cam_handle, 0);
	}
	*mode = camera->current_mode;
	return ERR_OK;
}

int
start_acqui(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;
	device_start(camera);
	return ERR_OK;
}

int
stop_acqui(void *cam_handle)
{
	cam_v4l2_t *camera = cam_handle;
	device_stop(camera);
	return ERR_OK;
}

int
get_frame(void *cam_handle, image_t * img)
{
	cam_v4l2_t *camera = cam_handle;
	device_get_frame(camera);
	// TODO
	return ERR_OK;
}
