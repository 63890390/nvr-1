#ifndef NVR_CONFIG_FILE
#define NVR_CONFIG_FILE "config.ini"
#endif

#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "config.h"
#include "nvr.h"
#include "log.h"

char *config_file = NVR_CONFIG_FILE;
Settings settings;
pthread_t threads[sizeof(settings.cameras)];

void main_shutdown(int sig) {
    (void) (sig);
    nvr_log(NVR_LOG_INFO, "shutting down");
    for (unsigned int i = 0; i < settings.camera_count; i++)
        if (settings.cameras[i].running)
            settings.cameras[i].running = 0;
}

void read_configuration(int sig) {
    signal(SIGHUP, read_configuration);
    if (sig == SIGHUP)
        nvr_log(NVR_LOG_INFO, "reloading configuration file \"%s\"", config_file);
    if (read_config(config_file, &settings) < 0) {
        nvr_log(NVR_LOG_FATAL, "error reading \"%s\"", config_file);
        return;
    }
    nvr_log_set_level(settings.log_level);
    nvr_log_set_ffmpeg_level(settings.ffmpeg_log_level);
    nvr_log_close_file();
    nvr_log_open_file(settings.log_file);
    nvr_log(NVR_LOG_DEBUG, "storage_dir: %s", settings.storage_dir);
    nvr_log(NVR_LOG_DEBUG, "segment_length: %d", settings.segment_length);
    nvr_log(NVR_LOG_DEBUG, "retry_delay: %d", settings.retry_delay);
    nvr_log(NVR_LOG_DEBUG, "conn_timeout: %d", settings.conn_timeout);
    nvr_log(NVR_LOG_DEBUG, "recv_timeout: %d", settings.recv_timeout);
    nvr_log(NVR_LOG_DEBUG, "log_file: %s", settings.log_file);
    nvr_log(NVR_LOG_DEBUG, "log_level: %s", nvr_log_get_level_name(settings.log_level));
    nvr_log(NVR_LOG_DEBUG, "ffmpeg_log_level: %s", nvr_log_get_ffmpeg_level_name(settings.ffmpeg_log_level));
}

void *record_thread(void *args) {
    Camera *cam = args;
    free(args);
    while (cam->running) {
        NVRThreadParams params;
        record(cam, &settings, &params);
        if (cam->running) {
            nvr_log(NVR_LOG_WARNING, "%s reconnecting in %d milliseconds", cam->name, settings.retry_delay);
            int elapsed = 0;
            while (elapsed < settings.retry_delay && cam->running) {
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 10000;
                select(0, NULL, NULL, NULL, &tv);
                elapsed += 10;
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    signal(SIGTERM, main_shutdown);
    signal(SIGINT, main_shutdown);

    pthread_setspecific(0, "main");
    nvr_log(NVR_LOG_INFO, "starting");

    if (argc > 1)
        config_file = argv[1];
    read_configuration(0);
    ffmpeg_init();

    while (strlen(settings.cameras[settings.camera_count].name) > 0 &&
           settings.camera_count < sizeof(settings.cameras)) {
        nvr_log(NVR_LOG_DEBUG,
                "%s %s", settings.cameras[settings.camera_count].name, settings.cameras[settings.camera_count].uri);
        settings.cameras[settings.camera_count].running = 1;
        pthread_create(&threads[settings.camera_count], NULL, record_thread, &settings.cameras[settings.camera_count]);
        settings.camera_count++;
    }
    nvr_log(NVR_LOG_DEBUG, "camera_count: %d", settings.camera_count);

    for (unsigned int t = 0; t < settings.camera_count; t++)
        pthread_join(threads[t], NULL);

    ffmpeg_deinit();
    nvr_log_close_file();
    return 0;
}
