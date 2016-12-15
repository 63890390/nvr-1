#ifndef NVR_H
#define NVR_H

#include "types.h"
#include "sys/time.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) MAX(MAX(a,b),c)
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MIN3(a, b, c) MIN(MIN(a,b),c)

typedef struct {
    unsigned int *conn_timeout;
    unsigned int *recv_timeout;
    unsigned int *running;
    unsigned int connected;
    unsigned int failed;
    struct timeval start_time;
    struct timeval last_packet_time;
} NVRThreadParams;

void ffmpeg_init();

void ffmpeg_deinit();

int record(Camera *camera, Settings *settings, NVRThreadParams *params);

#endif //NVR_H
