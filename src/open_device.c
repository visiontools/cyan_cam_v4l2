#include <errno.h>
#include <fcntl.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cam_v4l2.h"

void open_device( cam_v4l2_t* cam ) {

    struct stat st;

    if ( stat(cam->dev_name, &st ) == -1 ) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\\n",
                cam->dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no devicen", cam->dev_name);
        exit(EXIT_FAILURE);
    }

    cam->fd = open(cam->dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (cam->fd == -1) {
        fprintf(stderr, "Cannot open '%s': %d, %s\\n",
                cam->dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

}
