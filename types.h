#ifndef NVR_TYPES_H
#define NVR_TYPES_H

typedef struct {
    char uri[256];
    char name[64];
    int running;
    int output_open;
} Camera;

typedef struct {
    int segment_length;
    int retry_delay;
    unsigned int camera_count;
    int log_level;
    int ffmpeg_log_level;
    char storage_dir[1024];
    char log_file[1024];
    Camera cameras[1024];
} Settings;

#endif //NVR_TYPES_H
