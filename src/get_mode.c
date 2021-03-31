#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "cam_v4l2.h"

#define VERBOSE

void get_supported_v4l_formats( cam_v4l2_t *cam, 
                                int v4l_formats[100], int* nb_formats ) ;

void get_supported_v4l_size( cam_v4l2_t *cam, int format_index,
                                int sizes_width[100], int size_height[100], int* nb_size ) ;

void get_supported_v4l_fps( cam_v4l2_t *cam, int format_index, int width, int height,
                                int fps_numerator[100], int fps_denominator[100], int* nb_fps) ;

void get_supported_v4l_formats( cam_v4l2_t *cam, 
                                int formats[100], int* nb_formats ) {
    struct v4l2_fmtdesc fmtdesc ;

    memset(&fmtdesc, 0, sizeof(fmtdesc)) ;
    fmtdesc.index = 0 ;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;

    *nb_formats = 0 ;

    while (  0 == xioctl( cam->fd, VIDIOC_ENUM_FMT, &fmtdesc ) ) {
        formats[*nb_formats] = fmtdesc.pixelformat ;
        (*nb_formats)++ ;
        fmtdesc.index++ ;
    }
}

void get_supported_v4l_size( cam_v4l2_t *cam, int format_index,
                                int sizes_width[100], int sizes_height[100], int* nb_size ) {

    struct v4l2_frmsizeenum frmsizeenum ;
    
    *nb_size = 0 ;

    memset(&frmsizeenum, 0, sizeof(frmsizeenum)) ;
    frmsizeenum.index = 0 ;
    frmsizeenum.pixel_format = format_index ;
    
    if  (  0 != xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
        fprintf(stderr, "ioctl error %d, %s\\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    switch( frmsizeenum.type ) {
        case V4L2_FRMSIZE_TYPE_DISCRETE :
            frmsizeenum.index = 0 ;
            while ( 0 == xioctl( cam->fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum ) ) {
                sizes_width [*nb_size] = frmsizeenum.discrete.width ; 
                sizes_height[*nb_size] = frmsizeenum.discrete.height; 
                (*nb_size)++ ;
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
}

void get_supported_v4l_fps( cam_v4l2_t *cam, int format_index, int width, int height,
                                int fps_numerator[100], int fps_denominator[100], int* nb_fps) {

    struct v4l2_frmivalenum frmivalenum ;
    
    *nb_fps = 0 ;

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
                fps_numerator[*nb_fps] = frmivalenum.discrete.numerator ;
                fps_denominator[*nb_fps] = frmivalenum.discrete.denominator ;
                (*nb_fps)++ ;
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
}


void get_modes( cam_v4l2_t *cam ) {

    int formats[100] ;
    int colorspace[100] ;
    int width[100] ;
    int height[100] ;
    int fps_numerator[100] ;
    int fps_denominator[100] ;
    int nb_formats ;
    int nb_sizes ;
    int nb_fps ;
    int i,j,i_fmt, i_size ;

    // Count available modes

    cam->v4l_nb_modes = 0 ;
    get_supported_v4l_formats( cam, formats, &nb_formats ) ;
    for ( i_fmt=0; i_fmt<nb_formats; i_fmt++ ) {
        get_supported_v4l_size( cam, formats[i_fmt], width, height, &nb_sizes ) ;
        for (i_size=0; i_size < nb_sizes; i_size++ ) {
            get_supported_v4l_fps( cam, formats[i_fmt], width[i_size], height[i_size], fps_numerator, fps_denominator, &nb_fps ) ;
            cam->v4l_nb_modes += nb_fps ;
        }   
    }

    // Allocate arrays
    cam->v4l_modes = (v4lmode_t*) malloc( cam->v4l_nb_modes*sizeof(v4lmode_t) ) ;

    // Fill v4l_mode array

    cam->v4l_nb_modes=0;
    get_supported_v4l_formats( cam, formats, &nb_formats ) ;
    for ( i_fmt=0; i_fmt< nb_formats; i_fmt++ ) {
        get_supported_v4l_size( cam, formats[i_fmt], width, height, &nb_sizes ) ;
        for (i_size=0; i_size < nb_sizes; i_size++ ) {
            get_supported_v4l_fps( cam, formats[i_fmt], width[i_size], height[i_size], fps_numerator, fps_denominator, &nb_fps ) ;
            for (j=0; j<nb_fps; j++) {
                cam->v4l_modes[cam->v4l_nb_modes+j].v4l_format = formats[i_fmt] ;
                cam->v4l_modes[cam->v4l_nb_modes+j].v4l_width = width[i_size] ;
                cam->v4l_modes[cam->v4l_nb_modes+j].v4l_height = height[i_size] ;
                cam->v4l_modes[cam->v4l_nb_modes+j].v4l_fps_numerator = fps_numerator[j] ;
                cam->v4l_modes[cam->v4l_nb_modes+j].v4l_fps_denominator = fps_denominator[j] ;
            }
            cam->v4l_nb_modes+=j ;
        }   
    }

    // Fill cyan mode arrays

    cam->nb_modes = 0 ;
    cam->modes    = (hw_mode_t*) malloc( cam->v4l_nb_modes * sizeof( hw_mode_t ) ) ;
    cam->matching = (int*) malloc( cam->v4l_nb_modes * sizeof( int ) ) ;
    for ( i=0; i<cam->v4l_nb_modes; i++ ) {

        switch ( cam->v4l_modes[i].v4l_format ) {
            case V4L2_PIX_FMT_YUYV:
                cam->modes[cam->nb_modes].pixel_format = YUV422_8 ;
                cam->modes[cam->nb_modes].resolution.cols = cam->v4l_modes[i].v4l_width ;
                cam->modes[cam->nb_modes].resolution.rows = cam->v4l_modes[i].v4l_height ;
                cam->modes[cam->nb_modes].fps = ((float) cam->v4l_modes[i].v4l_fps_denominator) / ((float) cam->v4l_modes[i].v4l_fps_numerator) ; 
                cam->matching[cam->nb_modes] = i ;
                cam->nb_modes++ ;
                break ;
            default:
                
                break ;
        }

    }

    printf("%d modes supported by v4l\n", cam->v4l_nb_modes ) ;
    printf("%d modes supported by cyan\n", cam->nb_modes ) ;

}
