#ifndef NVR_CONFIG_FILE
#define NVR_CONFIG_FILE "config.ini"
#endif
#ifndef NVR_MAX_CAMERAS
#define NVR_MAX_CAMERAS 1024
#endif

#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "types.h"
#include "config.h"
#include "nvr.h"
#include "log.h"

char *config_file = NVR_CONFIG_FILE;
Settings settings;
pthread_t threads[2048];

void main_shutdown(int sig) {
    (void) (sig);
    nvr_log(NVR_LOG_INFO, "shutting down");
    for (int i = 0; i < settings.camera_count; i++)
        if (settings.cameras[i].running) {
            settings.cameras[i].running = 0;
            /*
            if (!settings.cameras[i].output_open)
                pthread_cancel(threads[i]);
            */
        }
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
    nvr_log(NVR_LOG_DEBUG, "segment_length: %i", settings.segment_length);
    nvr_log(NVR_LOG_DEBUG, "retry_delay: %i", settings.retry_delay);
    nvr_log(NVR_LOG_DEBUG, "camera_count: %i", settings.camera_count);
    nvr_log(NVR_LOG_DEBUG, "log_file: %s", settings.log_file);
    nvr_log(NVR_LOG_DEBUG, "log_level: %s", nvr_log_get_level_name(settings.log_level));
    nvr_log(NVR_LOG_DEBUG, "ffmpeg_log_level: %s", nvr_log_get_ffmpeg_level_name(settings.ffmpeg_log_level));
}

void *record_thread(void *args) {
    Camera *cam = args;
    while (cam->running) {
        record(cam, &settings);
        if (cam->running) {
            nvr_log(NVR_LOG_WARNING, "%s reconnecting in %d seconds", cam->name, settings.retry_delay);
            sleep((unsigned int) settings.retry_delay);
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

    while (strlen(settings.cameras[settings.camera_count].name) > 0 && settings.camera_count < 1024) {
        nvr_log(NVR_LOG_DEBUG,
                "%s %s", settings.cameras[settings.camera_count].name, settings.cameras[settings.camera_count].uri);
        settings.cameras[settings.camera_count].running = 1;
        pthread_create(&threads[settings.camera_count], NULL, record_thread, &settings.cameras[settings.camera_count]);
        settings.camera_count++;
    }

    for (int t = 0; t < settings.camera_count; t++)
        pthread_join(threads[t], NULL);

    ffmpeg_deinit();
    nvr_log_close_file();
    return 0;
}
