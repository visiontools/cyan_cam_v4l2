#ifndef CAM_V4L2_H
#define CAM_V4L2_H

#include <cyan/hwcam/plugin.h>
#include <cyan/common/error.h>

#define VERBOSE

enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

struct buffer {
	void *start;
	size_t length;
};

typedef struct {
	int v4l_format;
	int v4l_width;
	int v4l_height;
	int v4l_fps_numerator;
	int v4l_fps_denominator;
	char description[100];
} v4lmode_t;

typedef struct {

	// v4l2 stuff

	char *dev_name;
	int fd;
	enum io_method io;
	struct buffer *buffers;
	unsigned int n_buffers;

	// V4l modes

	hw_mode_t *modes;
	v4lmode_t *v4l_modes;
	int nb_modes;
	int current_mode;

} cam_v4l2_t;

int xioctl(int fh, int request, void *arg);

void device_init(cam_v4l2_t * cam);
void device_uninit(cam_v4l2_t * cam);
void device_open(cam_v4l2_t * cam);
void device_close(cam_v4l2_t * cam);
void device_start(cam_v4l2_t * cam);
void device_stop(cam_v4l2_t * cam);
void device_get_frame(cam_v4l2_t * cam);
void device_get_available_modes(cam_v4l2_t * cam);

#endif
