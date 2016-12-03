#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct {
    int running;
    int segment_length;
    char storage_dir[256];
} Settings;

#endif //SETTINGS_H

#ifndef NVR_SETTINGS_H
#define NVR_SETTINGS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "nvr.h"
#include "jsmn.h"

int read_config(char *config_file, Settings *settings, Camera cameras[256]);

int read_json(char *config_file, Settings *settings, Camera cameras[256]);

int read_ini(char *config_file, Settings *settings, Camera cameras[256]);

#endif //NVR_SETTINGS_H
