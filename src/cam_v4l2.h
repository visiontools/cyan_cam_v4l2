#ifndef CAM_V4L2_H
#define CAM_V4L2_H

#include <stdlib.h>
#include <cyan/hwcam/plugin.h>
#include <cyan/common/error.h>

enum io_method {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

struct buffer {
    void   *start;
    size_t  length;
};

typedef struct { 
    int v4l_format ;
    int v4l_width ;
    int v4l_height ;
    int v4l_fps_numerator ;
    int v4l_fps_denominator ;
    int   cyan_export ;
} v4lmode_t ;

typedef struct {

    // v4l2 stuff

    char                   *dev_name;
    int                     fd ;
    enum io_method          io ;
    struct buffer          *buffers;
    unsigned int            n_buffers;

    // Acquisition modes
    
    hw_mode_t *modes ;
    v4lmode_t *v4l_modes ;
    int       nb_modes ;
    int       current_mode ;

} cam_v4l2_t ;


int xioctl(int fh, int request, void *arg) ;

void open_device( cam_v4l2_t* cam ) ;
void init_device(cam_v4l2_t* cam) ;
void uninit_device(cam_v4l2_t *cam) ;
void close_device(cam_v4l2_t *cam) ;
void start_device(cam_v4l2_t *cam) ;
void stop_device(cam_v4l2_t *cam) ;
void get_frame_device(cam_v4l2_t *cam) ;


#endif
