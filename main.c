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
int running = 1;
Settings settings;
int camera_count = 0;
Camera curr_cams[sizeof(settings.cameras) / sizeof(*settings.cameras)];
pthread_key_t thread_name_key;

void main_shutdown(int sig) {
    (void) (sig);
    nvr_log(NVR_LOG_INFO, "shutting down");
    running = 0;
    for (int i = 0; i < sizeof(curr_cams) / sizeof(*curr_cams); i++)
        if (curr_cams[i].running)
            curr_cams[i].running = 0;
}

void *record_thread(void *args) {
    Camera *cam = args;
    while (cam->running) {
        NVRThreadParams params;
        pthread_setspecific(thread_name_key, cam->name);
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

void cleanup_cams() {
    for (int a = 0; a < sizeof(curr_cams) / sizeof(*curr_cams); a++) {
        if (strlen(curr_cams[a].name) && strlen(curr_cams[a].uri)) {
            int found = 0;
            for (int b = 0; b < sizeof(settings.cameras) / sizeof(*settings.cameras); b++) {
                if (strlen(settings.cameras[b].name) && strlen(settings.cameras[b].uri)) {
                    if (strncmp(curr_cams[a].name, settings.cameras[b].name, sizeof(curr_cams[a].name)) == 0 &&
                        strncmp(curr_cams[a].uri, settings.cameras[b].uri, sizeof(curr_cams[a].uri)) == 0) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 0) {
                nvr_log(NVR_LOG_INFO, "obsolete camera: %s", curr_cams[a].name);
                curr_cams[a].running = 0;
                pthread_join(curr_cams[a].thread, NULL);
                strncpy(curr_cams[a].name, "", sizeof(settings.cameras[a].name));
                strncpy(curr_cams[a].uri, "", sizeof(settings.cameras[a].uri));
                camera_count--;
            }
        }
    }
}

void add_cams() {
    for (int a = 0; a < sizeof(settings.cameras) / sizeof(*settings.cameras); a++) {
        if (strlen(settings.cameras[a].name) && strlen(settings.cameras[a].uri)) {
            int found = 0;
            for (int b = 0; b < sizeof(curr_cams) / sizeof(*curr_cams); b++) {
                if (strlen(curr_cams[b].name) && strlen(curr_cams[b].uri)) {
                    if (strncmp(settings.cameras[a].name, curr_cams[b].name, sizeof(settings.cameras[a].name)) == 0 &&
                        strncmp(settings.cameras[a].uri, curr_cams[b].uri, sizeof(settings.cameras[a].uri)) == 0) {
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 0) {
                int added = 0;
                nvr_log(NVR_LOG_INFO, "new camera: %s", settings.cameras[a].name);
                for (int c = 0; c < sizeof(curr_cams) / sizeof(*curr_cams); c++) {
                    if (!strlen(curr_cams[c].name) && !strlen(curr_cams[c].uri)) {
                        strncpy(curr_cams[c].name, settings.cameras[a].name, sizeof(settings.cameras[a].name));
                        strncpy(curr_cams[c].uri, settings.cameras[a].uri, sizeof(settings.cameras[a].uri));
                        added = 1;
                        curr_cams[a].running = 1;
                        pthread_create(&curr_cams[a].thread, NULL, record_thread, &curr_cams[a]);
                        camera_count++;
                        break;
                    }
                }
                if (!added) {
                    nvr_log(NVR_LOG_ERROR, "camera limit exceeded");
                    break;
                }
            }
        }
    }
}

void read_configuration(int sig) {
    signal(SIGHUP, read_configuration);
    if (sig == SIGHUP)
        nvr_log(NVR_LOG_INFO, "reloading configuration file \"%s\"", config_file);
    if (read_config(config_file, &settings) < 0) {
        nvr_log(NVR_LOG_ERROR, "error reading \"%s\"", config_file);
        return;
    }
    nvr_log_set_thread_name_key(thread_name_key);
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

    cleanup_cams();
    add_cams();
    for (int a = 0; a < sizeof(curr_cams) / sizeof(*curr_cams); a++)
        if (strlen(curr_cams[a].name) && strlen(curr_cams[a].uri))
            nvr_log(NVR_LOG_DEBUG, "camera: %s", curr_cams[a].name);
}

int main(int argc, char **argv) {
    signal(SIGTERM, main_shutdown);
    signal(SIGINT, main_shutdown);

    pthread_key_create(&thread_name_key, NULL);
    pthread_setspecific(thread_name_key, "main");

    nvr_log(NVR_LOG_INFO, "starting");

    if (argc > 1)
        config_file = argv[1];

    ffmpeg_init();

    read_configuration(0);

    nvr_log(NVR_LOG_DEBUG, "camera_count: %d", camera_count);

    while (running) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        select(0, NULL, NULL, NULL, &tv);
    }

    for (int a = 0; a < sizeof(curr_cams) / sizeof(*curr_cams); a++)
        if (curr_cams[a].thread)
            pthread_join(curr_cams[a].thread, NULL);

    ffmpeg_deinit();
    nvr_log_close_file();
    return 0;
}
