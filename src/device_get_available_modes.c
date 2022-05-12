#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <cyan/hwcam/pixelformats.h>
#include <cyan/hwcam/imageformats.h>

#include "cam_v4l2.h"

#define VERBOSE

void convert_v4l_mode_to_cyan( v4lmode_t*, hw_mode_t* ) ;


void convert_v4l_mode_to_cyan( v4lmode_t* v4lmode, hw_mode_t* cyanmode) {

    struct v4l2_fmtdesc fmtdesc ;
    struct v4l2_frmsizeenum frmsizeenum ;
    struct v4l2_frmivalenum frmivalenum ;
    
    cyanmode->cols = v4lmode->v4l_width ;
    cyanmode->rows = v4lmode->v4l_height ;
    cyanmode->fps  = ((float) v4lmode->v4l_fps_denominator) / ((float)v4lmode->v4l_fps_numerator) ;
    cyanmode->enabled = 0 ;
    snprintf(cyanmode->description, 100, "%s %dx%d @ %f FPS", v4lmode->description, cyanmode->cols, cyanmode->rows, cyanmode->fps ) ;   // FIXME

    switch (v4lmode->v4l_format) {                          // Add new formats here
        case V4L2_PIX_FMT_YUYV :
            cyanmode->pixel_format = YUV422_8_UYVY ; 
            cyanmode->image_format = FMT_PLANE ; 
            cyanmode->enabled = 1 ;
            break ;
        default:
            cyanmode->pixel_format = Unsupported; 
            cyanmode->image_format = FMT_UNSUPPORTED ; 
            cyanmode->enabled = 0 ;
            break ;
    }
}



void device_get_available_modes( cam_v4l2_t *cam ) {

    struct v4l2_fmtdesc fmtdesc ;
    struct v4l2_frmsizeenum frmsizeenum ;
    struct v4l2_frmivalenum frmivalenum ;
    int index ;
    
    cam->nb_modes = 0  ;

    // Iterate through all format / resolution / framerate and count them

    memset(&fmtdesc, 0, sizeof(fmtdesc)) ;
    fmtdesc.index = 0 ;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    while (  0 == xioctl( cam->fd, VIDIOC_ENUM_FMT, &fmtdesc ) ) {
        memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
        frmsizeenum.index = 0 ;
        frmsizeenum.pixel_format = fmtdesc.pixelformat ;
        if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
            fprintf(stderr, "[frmsizeenum error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        switch( frmsizeenum.type ) {
            case V4L2_FRMSIZE_TYPE_DISCRETE :
                memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
                frmsizeenum.index = 0 ;
                frmsizeenum.pixel_format = fmtdesc.pixelformat ;
                while ( 0 == xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
                    memset(&frmivalenum, 0, sizeof(frmivalenum)) ;
                    frmivalenum.index = 0 ;
                    frmivalenum.pixel_format = fmtdesc.pixelformat ;
                    frmivalenum.width = frmsizeenum.discrete.width ;
                    frmivalenum.height = frmsizeenum.discrete.height ;
                    if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum ) ) {
                        fprintf(stderr, "frmivalenum error %d, %s\\n", errno, strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    switch( frmivalenum.type ) {
                        case V4L2_FRMIVAL_TYPE_DISCRETE :
                            frmivalenum.index = 0 ;
                            while ( 0 == xioctl( cam->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum ) ) {
                                cam->nb_modes++ ;           // Count it !!!!
                                frmivalenum.index++  ;
                            }
                            break ;
                        case V4L2_FRMIVAL_TYPE_STEPWISE :
                            printf("Stepwise size values \n") ;
                            break ;
                        case V4L2_FRMIVAL_TYPE_CONTINUOUS :
                            printf("Continuous size values \n") ;
                            break ;
                        default :
                            printf("This should never happen \n") ;
                            break ;
                    }
                    frmsizeenum.index++  ;
                }
                break ;
            case V4L2_FRMSIZE_TYPE_STEPWISE :
                printf("Stepwise size values \n") ;
                break ;
            case V4L2_FRMSIZE_TYPE_CONTINUOUS :
                printf("Continuous size values \n") ;
                break ;
            default :
                printf("This should never happen \n") ;
                break ;
        }
        fmtdesc.index++ ;
    }

    // Array allocation

    cam->modes = (hw_mode_t*) malloc(cam->nb_modes*sizeof(hw_mode_t)) ;
    cam->v4l_modes = (v4lmode_t*) malloc(cam->nb_modes*sizeof(v4lmode_t)) ;
    
    // Array filling

    index = 0 ;
    memset(&fmtdesc, 0, sizeof(fmtdesc)) ;
    fmtdesc.index = 0 ;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    while (  0 == xioctl( cam->fd, VIDIOC_ENUM_FMT, &fmtdesc ) ) {
        memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
        frmsizeenum.index = 0 ;
        frmsizeenum.pixel_format = fmtdesc.pixelformat ;
        if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
            fprintf(stderr, "[frmsizeenum error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        switch( frmsizeenum.type ) {
            case V4L2_FRMSIZE_TYPE_DISCRETE :
                memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
                frmsizeenum.index = 0 ;
                frmsizeenum.pixel_format = fmtdesc.pixelformat ;
                while ( 0 == xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
                    memset(&frmivalenum, 0, sizeof(frmivalenum)) ;
                    frmivalenum.index = 0 ;
                    frmivalenum.pixel_format = fmtdesc.pixelformat ;
                    frmivalenum.width = frmsizeenum.discrete.width ;
                    frmivalenum.height = frmsizeenum.discrete.height ;
                    if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum ) ) {
                        fprintf(stderr, "frmivalenum error %d, %s\\n", errno, strerror(errno));
                        exit(EXIT_FAILURE);
                    }
                    switch( frmivalenum.type ) {
                        case V4L2_FRMIVAL_TYPE_DISCRETE :
                            frmivalenum.index = 0 ;
                            while ( 0 == xioctl( cam->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum ) ) {
                                // --- Fill arrays
                                   cam->v4l_modes[index].v4l_format = fmtdesc.pixelformat ;
                                   snprintf(cam->v4l_modes[index].description, 100, "%s", fmtdesc.description ) ;
                                   cam->v4l_modes[index].v4l_width  = frmsizeenum.discrete.width ;
                                   cam->v4l_modes[index].v4l_height = frmsizeenum.discrete.height ;
                                   cam->v4l_modes[index].v4l_fps_numerator    = frmivalenum.discrete.numerator ;
                                   cam->v4l_modes[index].v4l_fps_denominator  = frmivalenum.discrete.denominator ;
                                   convert_v4l_mode_to_cyan( &(cam->v4l_modes[index]),&(cam->modes[index]) ) ;
                                   index++ ;                   
                                // ---
                                frmivalenum.index++  ;
                            }
                            break ;
                        case V4L2_FRMIVAL_TYPE_STEPWISE :
                            printf("Stepwise size values \n") ;
                            break ;
                        case V4L2_FRMIVAL_TYPE_CONTINUOUS :
                            printf("Continuous size values \n") ;
                            break ;
                        default :
                            printf("This should never happen \n") ;
                            break ;
                    }
                    frmsizeenum.index++  ;
                }
                break ;
            case V4L2_FRMSIZE_TYPE_STEPWISE :
                printf("Stepwise size values \n") ;
                break ;
            case V4L2_FRMSIZE_TYPE_CONTINUOUS :
                printf("Continuous size values \n") ;
                break ;
            default :
                printf("This should never happen \n") ;
                break ;
        }
        fmtdesc.index++ ;
    }
    

}
