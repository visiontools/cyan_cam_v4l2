#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <cyan/hwcam/plugin.h>
#include <cyan/hwcam/modes.h>
#include <cyan/common/error.h>

#include "cam_v4l2.h"

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

    camera->dev_name = malloc( 255 ) ;
    strncpy( camera->dev_name, filename, 255 ) ;

    open_device( camera ) ;     
    init_device( camera ) ;

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

    return ERR_OK ;
}

int get_serial( void* cam_handle, char** serial, size_t* serial_size )  {

    return ERR_OK ;
}

int set_mode( void* cam_handle, int mode )  {
    
    return ERR_OK ;
}

int get_mode( void* cam_handle, int* mode ) {

    return ERR_OK ;
}

int start_acqui ( void* cam_handle ) {

    return ERR_OK ;
}

int stop_acqui ( void* cam_handle ) {

    return ERR_OK ;
}

int get_frame ( void* cam_handle, image_t* img ) {
    
    return ERR_OK ;
}
