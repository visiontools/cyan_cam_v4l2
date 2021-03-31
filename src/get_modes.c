#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <cyan/hwcam/pixelformats.h>

#include "cam_v4l2.h"

#define VERBOSE

void get_modes( cam_v4l2_t *cam ) {

    struct v4l2_fmtdesc fmtdesc ;
    struct v4l2_frmsizeenum frmsizeenum ;
    struct v4l2_frmivalenum frmivalenum ;
    
    cam->nb_modes = 0  ;

    // Iterate through all format / resolution / framerate and count them

    memset(&fmtdesc, 0, sizeof(fmtdesc)) ;
    fmtdesc.index = 0 ;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    while (  0 == xioctl( cam->fd, VIDIOC_ENUM_FMT, &fmtdesc ) ) {
        memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
        frmsizeenum.index = 0 ;
        frmsizeenum.pixel_format = format_index ;
        if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
            fprintf(stderr, "ioctl error %d, %s\\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        switch( frmsizeenum.type ) {
            case V4L2_FRMSIZE_TYPE_DISCRETE :
                memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
                frmsizeenum.index = 0 ;
                frmsizeenum.pixel_format = format_index ;
                while ( 0 == xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
                    memset(&frmivalenum, 0, sizeof(frmivalenum)) ;
                    frmivalenum.index = 0 ;
                    frmivalenum.pixel_format = format_index ;
                    frmivalenum.width = width ;
                    frmivalenum.height = height ;
                    if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum ) ) {
                        fprintf(stderr, "ioctl error %d, %s\\n", errno, strerror(errno));
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
    
    printf( "Found %d V4L2 acquisition modes \n", cam->nb_modes ) ;





}
