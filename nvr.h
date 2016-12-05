#ifndef NVR_H
#define NVR_H

#include <libavformat/avformat.h>
#include "types.h"
#include "utils.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) MAX(MAX(a,b),c)
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define MIN3(a,b,c) MIN(MIN(a,b),c)
int record(Camera *camera, Settings *settings);

#endif //NVR_H
