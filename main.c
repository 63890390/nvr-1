#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "types.h"
#include "config.h"
#include "nvr.h"

#ifndef NVR_CONFIG_FILE
#define NVR_CONFIG_FILE "config.ini"
#endif

char *config_file = NVR_CONFIG_FILE;
Settings settings;
int camera_count = 0;
pthread_t threads[256];

void main_shutdown(int sig) {
    (void) (sig);
    printf("\rshutting down\n");
    for (int i = 0; i < camera_count; i++)
        if (settings.cameras[i].running) {
            settings.cameras[i].running = 0;
            /*
            if (!settings.cameras[i].output_open)
                pthread_cancel(threads[i]);
            */
        }
}

void read_configuration(int sig) {
    (void) (sig);
    signal(SIGHUP, read_configuration);
    if (read_config(config_file, &settings) < 0) {
        printf("error reading \"%s\"\n", config_file);
        return;
    }
    printf("storage_dir: %s\n", settings.storage_dir);
    printf("segment_length: %i\n", settings.segment_length);
}

void *record_thread(void *args) {
    Camera *cam = args;
    while (cam->running) {
        record(cam, &settings);
        if (cam->running)
            sleep((unsigned int) settings.retry_delay);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int t;
    jsmn_parser parser;
    jsmn_init(&parser);

    signal(SIGTERM, main_shutdown);
    signal(SIGINT, main_shutdown);

    if (argc > 1)
        config_file = argv[1];
    read_configuration(0);

    av_log_set_level(AV_LOG_INFO);
    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    while (strlen(settings.cameras[camera_count].name) > 0) {
        printf("camera \"%s\" \"%s\"\n", settings.cameras[camera_count].name, settings.cameras[camera_count].uri);
        settings.cameras[camera_count].running = 1;
        pthread_create(&threads[camera_count], NULL, record_thread, &settings.cameras[camera_count]);
        camera_count++;
    }

    for (t = 0; t < camera_count; t++)
        pthread_join(threads[t], NULL);

    avformat_network_deinit();
    return 0;
}
