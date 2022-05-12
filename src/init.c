#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cyan/hwcam/pixelformats.h>
#include <cyan/hwcam/imageformats.h>

#include "cam_v4l2.h"



int init_read(cam_v4l2_t * cam, unsigned int buffer_size);
int init_userp(cam_v4l2_t * cam, unsigned int buffer_size);
int init_mmap(cam_v4l2_t * cam);
void convert_v4l_mode_to_cyan(v4lmode_t *, hw_mode_t *);

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

    //--- Structure Allocation and init.
    
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

    //--- Device opening
    
	struct stat st;

	if (stat(camera->dev_name, &st) == -1) {
		fprintf(stderr, "[cam_v4l2] Cannot identify '%s': %d, %s\\n",
		    camera->dev_name, errno, strerror(errno));
		return ERR_NOPE;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "[cam_v4l2] %s is no device\n", camera->dev_name);
		return ERR_NOPE;
	}

	camera->fd = open(camera->dev_name, O_RDWR | O_NONBLOCK, 0);

	if (camera->fd == -1) {
		fprintf(stderr, "[cam_v4l2] Cannot open '%s': %d, %s\\n",
		    camera->dev_name, errno, strerror(errno));
		return ERR_NOPE;
	}

    //--- Device Initialization

	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

	unsigned int min;

	if (-1 == xioctl(camera->fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\\n",
			    camera->dev_name);
			return ERR_NOPE;
		} else {
			fprintf(stderr, "VIDIOC_QUERYCAP error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\\n",
		    camera->dev_name);
		return ERR_NOPE;
	}

	switch (camera->io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\\n",
			    camera->dev_name);
			return ERR_NOPE;
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\\n",
			    camera->dev_name);
			return ERR_NOPE;
		}
		break;
	}

	/* Select video input, video standard and tune here. */

	memset(&cropcap, 0, sizeof(cropcap));

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(camera->fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	/* reset to default */

		if (-1 == xioctl(camera->fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	memset(&fmt, 0, sizeof(fmt));

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/* Preserve original settings as set by v4l2-ctl for example */
	if (-1 == xioctl(camera->fd, VIDIOC_G_FMT, &fmt)) {
		fprintf(stderr, "VIDIOC_G_FMT error %d, %s\\n", errno,
		    strerror(errno));
		return ERR_NOPE;
	}

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (camera->io) {
	case IO_METHOD_READ:
		init_read(camera, fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap(camera);
		break;

	case IO_METHOD_USERPTR:
		init_userp(camera, fmt.fmt.pix.sizeimage);
		break;
	}

    //--- List availables modes
    
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_frmsizeenum frmsizeenum;
	struct v4l2_frmivalenum frmivalenum;
	int index;

	camera->nb_modes = 0;

	// Iterate through all format / resolution / framerate and count them

	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (0 == xioctl(camera->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
		memset(&frmsizeenum, 0, sizeof(frmsizeenum));
		frmsizeenum.index = 0;
		frmsizeenum.pixel_format = fmtdesc.pixelformat;
		if (0 != xioctl(camera->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum)) {
			fprintf(stderr, "[frmsizeenum error %d, %s\\n", errno,
			    strerror(errno));
			return ERR_NOPE;
		}
		switch (frmsizeenum.type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			memset(&frmsizeenum, 0, sizeof(frmsizeenum));
			frmsizeenum.index = 0;
			frmsizeenum.pixel_format = fmtdesc.pixelformat;
			while (0 == xioctl(camera->fd, VIDIOC_ENUM_FRAMESIZES,
				&frmsizeenum)) {
				memset(&frmivalenum, 0, sizeof(frmivalenum));
				frmivalenum.index = 0;
				frmivalenum.pixel_format = fmtdesc.pixelformat;
				frmivalenum.width = frmsizeenum.discrete.width;
				frmivalenum.height =
				    frmsizeenum.discrete.height;
				if (0 != xioctl(camera->fd,
					VIDIOC_ENUM_FRAMEINTERVALS,
					&frmivalenum)) {
					fprintf(stderr,
					    "frmivalenum error %d, %s\\n",
					    errno, strerror(errno));
					return ERR_NOPE;
				}
				switch (frmivalenum.type) {
				case V4L2_FRMIVAL_TYPE_DISCRETE:
					frmivalenum.index = 0;
					while (0 == xioctl(camera->fd,
						VIDIOC_ENUM_FRAMEINTERVALS,
						&frmivalenum)) {
						camera->nb_modes++;	// Count it !!!!
						frmivalenum.index++;
					}
					break;
				case V4L2_FRMIVAL_TYPE_STEPWISE:
					printf("Stepwise size values \n");
					break;
				case V4L2_FRMIVAL_TYPE_CONTINUOUS:
					printf("Continuous size values \n");
					break;
				default:
					printf("This should never happen \n");
					break;
				}
				frmsizeenum.index++;
			}
			break;
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			printf("Stepwise size values \n");
			break;
		case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			printf("Continuous size values \n");
			break;
		default:
			printf("This should never happen \n");
			break;
		}
		fmtdesc.index++;
	}

	// Array allocation

	camera->modes = (hw_mode_t *) malloc(camera->nb_modes * sizeof(hw_mode_t));
	camera->v4l_modes =
	    (v4lmode_t *) malloc(camera->nb_modes * sizeof(v4lmode_t));

	// Array filling

	index = 0;
	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (0 == xioctl(camera->fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
		memset(&frmsizeenum, 0, sizeof(frmsizeenum));
		frmsizeenum.index = 0;
		frmsizeenum.pixel_format = fmtdesc.pixelformat;
		if (0 != xioctl(camera->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum)) {
			fprintf(stderr, "[frmsizeenum error %d, %s\\n", errno,
			    strerror(errno));
			return ERR_NOPE;
		}
		switch (frmsizeenum.type) {
		case V4L2_FRMSIZE_TYPE_DISCRETE:
			memset(&frmsizeenum, 0, sizeof(frmsizeenum));
			frmsizeenum.index = 0;
			frmsizeenum.pixel_format = fmtdesc.pixelformat;
			while (0 == xioctl(camera->fd, VIDIOC_ENUM_FRAMESIZES,
				&frmsizeenum)) {
				memset(&frmivalenum, 0, sizeof(frmivalenum));
				frmivalenum.index = 0;
				frmivalenum.pixel_format = fmtdesc.pixelformat;
				frmivalenum.width = frmsizeenum.discrete.width;
				frmivalenum.height =
				    frmsizeenum.discrete.height;
				if (0 != xioctl(camera->fd,
					VIDIOC_ENUM_FRAMEINTERVALS,
					&frmivalenum)) {
					fprintf(stderr,
					    "frmivalenum error %d, %s\\n",
					    errno, strerror(errno));
					return ERR_NOPE;
				}
				switch (frmivalenum.type) {
				case V4L2_FRMIVAL_TYPE_DISCRETE:
					frmivalenum.index = 0;
					while (0 == xioctl(camera->fd,
						VIDIOC_ENUM_FRAMEINTERVALS,
						&frmivalenum)) {
						// --- Fill arrays
						camera->
						    v4l_modes[index].v4l_format
						    = fmtdesc.pixelformat;
						snprintf(camera->
						    v4l_modes
						    [index].description, 100,
						    "%s", fmtdesc.description);
						camera->
						    v4l_modes[index].v4l_width
						    =
						    frmsizeenum.discrete.width;
						camera->
						    v4l_modes[index].v4l_height
						    =
						    frmsizeenum.
						    discrete.height;
						camera->
						    v4l_modes
						    [index].v4l_fps_numerator =
						    frmivalenum.
						    discrete.numerator;
						camera->
						    v4l_modes
						    [index].v4l_fps_denominator
						    =
						    frmivalenum.
						    discrete.denominator;
						convert_v4l_mode_to_cyan(&
						    (camera->v4l_modes[index]),
						    &(camera->modes[index]));
						index++;
						// ---
						frmivalenum.index++;
					}
					break;
				case V4L2_FRMIVAL_TYPE_STEPWISE:
					printf("Stepwise size values \n");
					break;
				case V4L2_FRMIVAL_TYPE_CONTINUOUS:
					printf("Continuous size values \n");
					break;
				default:
					printf("This should never happen \n");
					break;
				}
				frmsizeenum.index++;
			}
			break;
		case V4L2_FRMSIZE_TYPE_STEPWISE:
			printf("Stepwise size values \n");
			break;
		case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			printf("Continuous size values \n");
			break;
		default:
			printf("This should never happen \n");
			break;
		}
		fmtdesc.index++;
	}

    //--- Affichage

#ifdef VERBOSE
	printf("Found %d acquisition modes \n", camera->nb_modes);
	for (i = 0; i < camera->nb_modes; i++) {
		printf("[%3d]%c %s\n", i,
		    (camera->modes[i].enabled) ? '+' : '-',
		    camera->modes[i].description);
	}
#endif

	for (i = 0; i < camera->nb_modes; i++) {
		if (camera->modes[i].enabled == 1) {
			if (set_mode(camera, i) == ERR_OK)
				break;
		}
	}

	if (camera->current_mode == -1) {
		fprintf(stderr,
		    "[cam_v4l2] Could not find any compatible mode. Exiting. \n");
		return ERR_NOPE;
	}
#ifdef VERBOSE
	printf("Current Mode is %d \n", camera->current_mode);
#endif

	return ERR_OK;
}



int
init_mmap(cam_v4l2_t * cam)
{
	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
			    "memory mappingn", cam->dev_name);
			return ERR_NOPE;
		} else {
			fprintf(stderr, "VIDIOC_REQBUFS error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\\n",
		    cam->dev_name);
		return ERR_NOPE;
	}

	cam->buffers = calloc(req.count, sizeof(*(cam->buffers)));

	if (!cam->buffers) {
		fprintf(stderr, "Out of memory\\n");
		return ERR_NOPE;
	}

	for (cam->n_buffers = 0; cam->n_buffers < req.count;
	    ++(cam->n_buffers)) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = cam->n_buffers;

		if (-1 == xioctl(cam->fd, VIDIOC_QUERYBUF, &buf)) {
			fprintf(stderr, "VIDIOC_QUERYBUF error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}

		cam->buffers[cam->n_buffers].length = buf.length;
		cam->buffers[cam->n_buffers].start =
		    mmap(NULL /* start anywhere */ ,
		    buf.length, PROT_READ | PROT_WRITE /* required */ ,
		    MAP_SHARED /* recommended */ ,
		    cam->fd, buf.m.offset);

		if (MAP_FAILED == cam->buffers[cam->n_buffers].start) {
			fprintf(stderr, "mmap error %d, %s\\n", errno,
			    strerror(errno));
			return ERR_NOPE;
		}
	}
    return ERR_OK ;
}

int
init_userp(cam_v4l2_t * cam, unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(cam->fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
			    "user pointer i/on", cam->dev_name);
			return ERR_NOPE;
		} else {
			fprintf(stderr, "VIDIOC_REQBUFS error %d, %s\\n",
			    errno, strerror(errno));
			return ERR_NOPE;
		}
	}

	cam->buffers = calloc(4, sizeof(*(cam->buffers)));

	if (!cam->buffers) {
		fprintf(stderr, "Out of memory\\n");
		return ERR_NOPE;
	}

	for (cam->n_buffers = 0; cam->n_buffers < 4; ++(cam->n_buffers)) {
		cam->buffers[cam->n_buffers].length = buffer_size;
		cam->buffers[cam->n_buffers].start = malloc(buffer_size);

		if (!cam->buffers[cam->n_buffers].start) {
			fprintf(stderr, "Out of memory\\n");
			return ERR_NOPE;
		}
	}
    return ERR_OK;
}


int
init_read(cam_v4l2_t * cam, unsigned int buffer_size)
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


void
convert_v4l_mode_to_cyan(v4lmode_t * v4lmode, hw_mode_t * cyanmode)
{

	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_frmsizeenum frmsizeenum;
	struct v4l2_frmivalenum frmivalenum;

	cyanmode->cols = v4lmode->v4l_width;
	cyanmode->rows = v4lmode->v4l_height;
	cyanmode->fps =
	    ((float) v4lmode->v4l_fps_denominator) /
	    ((float) v4lmode->v4l_fps_numerator);
	cyanmode->enabled = 0;
	snprintf(cyanmode->description, 100, "%s %dx%d @ %f FPS", v4lmode->description, cyanmode->cols, cyanmode->rows, cyanmode->fps);	// FIXME

	switch (v4lmode->v4l_format) {	// Add new formats here
	case V4L2_PIX_FMT_YUYV:
		cyanmode->pixel_format = YUV422_8_UYVY;
		cyanmode->image_format = FMT_PLANE;
		cyanmode->enabled = 1;
		break;
	default:
		cyanmode->pixel_format = Unsupported;
		cyanmode->image_format = FMT_UNSUPPORTED;
		cyanmode->enabled = 0;
		break;
	}
}

