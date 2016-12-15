#ifndef NVR_CONFIG_H
#define NVR_CONFIG_H

#include "types.h"

#ifndef NVR_DEFAULT_SEGMENT_LENGTH
#define NVR_DEFAULT_SEGMENT_LENGTH 0
#endif
#ifndef NVR_DEFAULT_RETRY_DELAY
#define NVR_DEFAULT_RETRY_DELAY 10
#endif
#ifndef NVR_DEFAULT_STORAGE_DIR
#define NVR_DEFAULT_STORAGE_DIR "."
#endif
#ifndef NVR_DEFAULT_LOG_FILE
#define NVR_DEFAULT_LOG_FILE "/dev/null"
#endif
#ifndef NVR_DEFAULT_LOG_LEVEL
#define NVR_DEFAULT_LOG_LEVEL "info"
#endif
#ifndef NVR_DEFAULT_FFMPEG_LOG_LEVEL
#define NVR_DEFAULT_FFMPEG_LOG_LEVEL "error"
#endif

int read_config(char *config_file, Settings *settings);

#endif //NVR_CONFIG_H
