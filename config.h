#ifndef NVR_CONFIG_H
#define NVR_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "types.h"
#include "ini.h"
#include "jsmn.h"

#ifndef NVR_DEFAULT_SEGMENT_LENGTH
#define NVR_DEFAULT_SEGMENT_LENGTH 0
#endif
#ifndef NVR_DEFAULT_RETRY_DELAY
#define NVR_DEFAULT_RETRY_DELAY 10
#endif
#ifndef NVR_DEFAULT_STORAGE_DIR
#define NVR_DEFAULT_STORAGE_DIR "."
#endif

int read_config(char *config_file, Settings *settings);

#endif //NVR_CONFIG_H
