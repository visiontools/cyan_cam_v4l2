#include <stdio.h>
#include <sys/time.h>


#include <cyan/image/image.h>
#include <cyan/hwcam/hwcam.h>

#define NB_FRAMES 300



int main (int argc, char** argv) {

    char filename[250] ;
    hwcam_t* camera ;
    hw_mode_t*   modes ;
    int nb_modes ;
    int i ;
    image_t* image ;
    struct timeval start ;
    struct timeval stop ;

    if ( argc == 1 )
        camera = hwcam_new( "../libcam_v4l2.so", "/dev/video0" ) ;
    else
        camera = hwcam_new( "../libcam_v4l2.so", argv[1] ) ;
    
    hwcam_start_stream( camera ) ;
    
    gettimeofday(&start, NULL);

    for (i=0; i<NB_FRAMES; i++) {
        hwcam_dequeue( camera, &image ) ;
        sprintf(filename, "%04d.cyan", i ) ;
        image_save( image, filename ) ;
        hwcam_enqueue( camera, image ) ;
    }

    gettimeofday(&stop, NULL);

    hwcam_stop_stream( camera ) ;
    
    printf("Time for %d frames : %f sec \n", NB_FRAMES, 
            ( stop.tv_sec  + 0.000001 * stop.tv_usec ) -
            ( start.tv_sec + 0.000001 * start.tv_usec ) ) ;

    printf("FPS : %f \n", NB_FRAMES / ( 
            ( stop.tv_sec  + 0.000001 * stop.tv_usec ) -
            ( start.tv_sec + 0.000001 * start.tv_usec ) ) );

    hwcam_free( camera ) ;

    return 0 ;

}
