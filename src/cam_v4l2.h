#ifndef CAM_V4L2_H
#define CAM_V4L2_H

#include <stdlib.h>

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

    char                   *dev_name;
    int                     fd ;
    enum io_method          io ;
    struct buffer          *buffers;
    unsigned int            n_buffers;

} cam_v4l2_t ;


int xioctl(int fh, int request, void *arg) ;

void open_device( cam_v4l2_t* cam ) ;
void init_device(cam_v4l2_t* cam) ;
void uninit_device(cam_v4l2_t *cam) ;
void close_device(cam_v4l2_t *cam) ;


#endif
