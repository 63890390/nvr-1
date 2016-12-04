#ifndef NVR_SETTINGS_H
#define NVR_SETTINGS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "types.h"
#include "ini.h"
#include "jsmn.h"

int read_config(char *config_file, Settings *settings);

#endif //NVR_SETTINGS_H
