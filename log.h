#ifndef NVR_LOG_H
#define NVR_LOG_H

#include <stdarg.h>

#define NVR_LOG_FFMPEG -10
#define NVR_LOG_FATAL 0
#define NVR_LOG_ERROR 1
#define NVR_LOG_WARNING 2
#define NVR_LOG_INFO 3
#define NVR_LOG_DEBUG 4

void nvr_vlog(const int level, const char *format, va_list vl);

void nvr_log(const int level, const char *fmt, ...);

void nvr_log_ffmpeg(void *avcl, int level, const char *fmt, va_list vl);

void nvr_log_set_level(const int level);

void nvr_log_set_ffmpeg_level(const int level);

const char *nvr_log_get_level_name(const int level);

const char *nvr_log_get_ffmpeg_level_name(const int level);

void nvr_log_open_file(const char *filename);

void nvr_log_close_file();

#endif //NVR_LOG_H
