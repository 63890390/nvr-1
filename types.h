#ifndef NVR_TYPES_H
#define NVR_TYPES_H

typedef struct {
    char uri[256];
    char name[256];
    char running;
    char output_open;
} Camera;

typedef struct {
    int segment_length;
    int retry_delay;
    char storage_dir[256];
    Camera cameras[256];
} Settings;

#endif //NVR_TYPES_H
