#include <linux/videodev2.h>
#include <cyan/hwcam/pixelformats.h>
#include <cyan/hwcam/imageformats.h>
#include "cam_v4l2.h"

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
