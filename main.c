#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "types.h"
#include "config.h"
#include "nvr.h"

char *config_file = "config.ini";
Settings settings;
pthread_t threads[256];

void main_shutdown(int sig) {
    (void) (sig);
    printf("\rshutting down\n");
    for (int i = 0; i < 255; i++)
        if (settings.cameras[i].running)
            if (settings.cameras[i].output_open)
                settings.cameras[i].running = 0;
            else
                pthread_cancel(threads[i]);
    avformat_network_deinit();
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
    record(cam, &settings);
    return NULL;
}

int main(int argc, char **argv) {
    signal(SIGTERM, main_shutdown);
    signal(SIGINT, main_shutdown);

    int thread, camera_count = 0;
    jsmn_parser parser;
    jsmn_init(&parser);

    if (argc > 1)
        config_file = argv[1];
    read_configuration(0);

    av_log_set_level(AV_LOG_INFO);
    av_register_all();
    avformat_network_init();

    while (strlen(settings.cameras[camera_count].name) > 0) {
        printf("camera \"%s\" \"%s\"\n", settings.cameras[camera_count].name, settings.cameras[camera_count].uri);
        settings.cameras[camera_count].running = 1;
        pthread_create(&threads[camera_count], NULL, record_thread, &settings.cameras[camera_count]);
        camera_count++;
    }

    for (thread = 0; thread < camera_count; thread++)
        pthread_join(threads[thread], NULL);

    avformat_network_deinit();
    return 0;
}
