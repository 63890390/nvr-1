#ifndef NVR_TYPES_H
#define NVR_TYPES_H

typedef struct {
    char uri[256];
    char name[256];
} Camera;

typedef struct {
    int running;
    int segment_length;
    char storage_dir[256];
    Camera cameras[256];
} Settings;

#endif //NVR_TYPES_H
