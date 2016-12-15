#ifndef NVR_CONFIG_H
#define NVR_CONFIG_H

#include "types.h"

#define NVR_DEFAULT_SEGMENT_LENGTH 0
#define NVR_DEFAULT_RETRY_DELAY 10
#define NVR_DEFAULT_STORAGE_DIR "."
#define NVR_DEFAULT_LOG_FILE "/dev/null"
#define NVR_DEFAULT_LOG_LEVEL "info"
#define NVR_DEFAULT_FFMPEG_LOG_LEVEL "error"

int read_config(char *config_file, Settings *settings);

#endif //NVR_CONFIG_H
