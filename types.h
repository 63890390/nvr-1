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
    int camera_count;
    int log_level;
    int ffmpeg_log_level;
    char storage_dir[256];
    char log_file[256];
    Camera cameras[1024];
} Settings;

#endif //NVR_TYPES_H
