#include "log.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <libavutil/log.h>

FILE *nvr_log_fp;

int nvr_log_level = NVR_LOG_DEBUG;
int ffmpeg_log_level = AV_LOG_DEBUG;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

const char *nvr_log_get_ffmpeg_level_name(int level) {
    switch (level) {
        case AV_LOG_QUIET:
            return "quiet";
        case AV_LOG_DEBUG:
            return "debug";
        case AV_LOG_VERBOSE:
            return "verbose";
        case AV_LOG_INFO:
            return "info";
        case AV_LOG_WARNING:
            return "warning";
        case AV_LOG_ERROR:
            return "error";
        case AV_LOG_FATAL:
            return "fatal";
        case AV_LOG_PANIC:
            return "panic";
        default:
            return "";
    }
}

const char *nvr_log_get_level_name(int level) {
    switch (level) {
        case NVR_LOG_DEBUG:
            return "debug";
        case NVR_LOG_INFO:
            return "info";
        case NVR_LOG_WARNING:
            return "warning";
        case NVR_LOG_ERROR:
            return "error";
        case NVR_LOG_FATAL:
            return "fatal";
        case NVR_LOG_FFMPEG:
            return "ffmpeg";
        default:
            return "";
    }
}

void nvr_vlog(const int level, const char *format, va_list vl) {
    pthread_mutex_lock(&log_lock);
    if (level > nvr_log_level) {
        pthread_mutex_unlock(&log_lock);
        return;
    }
    time_t now;
    char date_str[64];
    char time_str[64];
    char msec_str[64];
    char timezone[64];
    char iso_date[64];
    char stderr_format[2048];
    char file_format[2048];
    struct timeval time_now;
    va_list vl1 = {};
    va_list vl2 = {};

    va_copy(vl1, vl);
    va_copy(vl2, vl);

    time(&now);
    gettimeofday(&time_now, NULL);

    strftime(date_str, sizeof date_str, "%F", localtime(&now));
    strftime(time_str, sizeof time_str, "%T", localtime(&now));
    strftime(timezone, sizeof timezone, "%z", localtime(&now));
    sprintf(msec_str, "%03d", (int) time_now.tv_usec / 1000);
    sprintf(iso_date, "%sT%s.%s%s", date_str, time_str, msec_str, timezone);

    sprintf(stderr_format, "\r%s [%s] [%s] %s\n",
            iso_date, (char *) pthread_getspecific(0), nvr_log_get_level_name(level), format);
    sprintf(file_format, "%s [%s] [%s] %s\n",
            iso_date, (char *) pthread_getspecific(0), nvr_log_get_level_name(level), format);
    vfprintf(stderr, stderr_format, vl1);
    if (nvr_log_fp != NULL) {
        vfprintf(nvr_log_fp, file_format, vl2);
        fflush(nvr_log_fp);
    }
    pthread_mutex_unlock(&log_lock);
}

void nvr_log(const int level, const char *fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    nvr_vlog(level, fmt, vl);
    va_end(vl);
}

void nvr_log_ffmpeg(void *avcl, int level, const char *fmt, va_list vl) {
    char nvr_format[2048];
    char class_name[64] = "";
    AVClass *avc = avcl ? *(AVClass **) avcl : NULL;
    if (level > ffmpeg_log_level)
        return;
    if (avc)
        strcpy(class_name, avc->class_name);
    vsprintf(nvr_format, fmt, vl);
    nvr_log(NVR_LOG_FFMPEG, "[%s] [%s] %s", class_name, nvr_log_get_ffmpeg_level_name(level), strtok(nvr_format, "\n"));
}

void nvr_log_set_level(const int level) {
    nvr_log_level = level;
}

void nvr_log_set_ffmpeg_level(const int level) {
    ffmpeg_log_level = level;
    av_log_set_level(level);
}

void nvr_log_open_file(const char *filename) {
    nvr_log_fp = fopen(filename, "a+");
}

void nvr_log_close_file() {
    if (nvr_log_fp != NULL) {
        fclose(nvr_log_fp);
        nvr_log_fp = NULL;
    }
}
