#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <linux/videodev2.h>
#include "cam_v4l2.h"

void get_modes( cam_v4l2_t *cam ) ;

// Cyan interface


int init( void** cam_handle, va_list args ){

    char* filename ;
    cam_v4l2_t* camera ;
    
    filename = va_arg(args, char*) ;

    printf("filename: %s \n", filename ) ;

    camera = (cam_v4l2_t*) malloc( sizeof(cam_v4l2_t ) ) ;
    *cam_handle = (void*) camera ;
    if ( camera == NULL ) {
        CYAN_ERROR( ERR_MALLOC ) ;
        return ERR_MALLOC ;
    }

    camera-> dev_name = NULL ;
    camera->fd = -1 ;
    camera->io = IO_METHOD_MMAP;
    camera->buffers = NULL ;
    camera->n_buffers = 0 ;
    camera->v4l_modes = NULL ;
    camera->modes = NULL ;
    camera->nb_modes = 0 ;
    camera->current_mode = -1 ;

    camera->dev_name = malloc( 255 ) ;
    strncpy( camera->dev_name, filename, 255 ) ;

    open_device( camera ) ;     
    init_device( camera ) ;
    get_modes( camera ) ;

    return ERR_OK ;
}


int deinit( void* cam_handle){
    cam_v4l2_t* camera = cam_handle ;
    
    uninit_device( camera ) ;
    close_device( camera ) ;

    free( cam_handle ) ;
    return ERR_OK ;
}

int get_available_modes( void* cam_handle, hw_mode_t** modes, int* nb_modes ) {
    cam_v4l2_t* camera = cam_handle ;
    if ( camera->modes == NULL ) 
        return ERR_NOPE ;
    if ( camera->nb_modes == 0 ) 
        return ERR_NOPE ;
    *modes = camera->modes ;
    *nb_modes = camera->nb_modes ;
    return ERR_OK ;
}

int get_serial( void* cam_handle, char** serial, size_t* serial_size )  {

    return ERR_NOPE ;
}

int set_mode( void* cam_handle, int mode )  {
    cam_v4l2_t* camera = cam_handle ;
    struct v4l2_format fmt;
    struct v4l2_streamparm parm;

    // Set Image mode

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = camera->v4l_modes[mode].v4l_width ;
    fmt.fmt.pix.height      = camera->v4l_modes[mode].v4l_height ;
    fmt.fmt.pix.pixelformat = camera->v4l_modes[mode].v4l_format ;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    xioctl(camera->fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != camera->v4l_modes[mode].v4l_format) {
        printf("Libv4l didn't accept format.\n");
        return ERR_NOPE;
    }

    // Set FPS

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator   = camera->v4l_modes[mode].v4l_fps_numerator ;
    parm.parm.capture.timeperframe.denominator = camera->v4l_modes[mode].v4l_fps_denominator ;

    if (xioctl(camera->fd, VIDIOC_S_PARM, &parm) < 0 ) {
        printf("Libv4l didn't accept framerate.\n");
        return ERR_NOPE;
    }

    // Set current_mode

    camera->current_mode = mode ;

    return ERR_OK ;
}

int get_mode( void* cam_handle, int* mode ) {
    cam_v4l2_t* camera = cam_handle ;
    if ( camera->current_mode == -1 ) {
        set_mode( cam_handle, 0 ) ;
    }
    *mode = camera->current_mode ;
    return ERR_OK ;
}

int start_acqui ( void* cam_handle ) {
    cam_v4l2_t* camera = cam_handle ;
    start_device( camera ) ;
    return ERR_OK ;
}

int stop_acqui ( void* cam_handle ) {
    cam_v4l2_t* camera = cam_handle ;
    stop_device( camera ) ;
    return ERR_OK ;
}

int get_frame ( void* cam_handle, image_t* img ) {
    cam_v4l2_t* camera = cam_handle ;
    get_frame_device( camera ) ;
    return ERR_OK ;
}
