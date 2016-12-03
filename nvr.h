#ifndef CAMERA_H
#define CAMERA_H

typedef struct {
    char uri[256];
    char name[256];
} Camera;

#endif //CAMERA_H

#ifndef NVR_NVR_H
#define NVR_NVR_H

#include <libavformat/avformat.h>
#include "settings.h"
#include "utils.h"

int record(Camera *camera, Settings *settings);

#endif //NVR_NVR_H
