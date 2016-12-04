#ifndef NVR_CONFIG_H
#define NVR_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "types.h"
#include "ini.h"
#include "jsmn.h"

int read_config(char *config_file, Settings *settings);

#endif //NVR_CONFIG_H
