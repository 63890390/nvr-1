#ifndef NVR_H
#define NVR_H

#include <libavformat/avformat.h>
#include "types.h"
#include "utils.h"

int record(Camera *camera, Settings *settings);

#endif //NVR_H
