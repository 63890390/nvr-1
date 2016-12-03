#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "nvr.h"

Settings settings;
Camera cameras[256];

void on_exit(int sig) {
    printf("\rshutting down\n");
    settings.running = 0;
}

void *read_stream_thread(void *args) {
    Camera *cam = args;
    record(cam, &settings);
    return NULL;
}

int main(int argc, char **argv) {
    int thread, camera_count = 0;
    signal(SIGTERM, on_exit);
    signal(SIGINT, on_exit);
    av_log_set_level(5);
    av_register_all();
    avformat_network_init();
    jsmn_parser parser;
    jsmn_init(&parser);
    pthread_t threads[256];
    read_config("config.json", &settings, cameras);
    settings.running = 1;
    printf("storage_dir: %s\n", settings.storage_dir);
    printf("segment_length: %i\n", settings.segment_length);


    while (strlen(cameras[camera_count].name) > 0) {
        printf("camera \"%s\" \"%s\"\n", cameras[camera_count].name, cameras[camera_count].uri);
        pthread_create(&threads[camera_count], NULL, read_stream_thread, &cameras[camera_count]);
        camera_count++;
    }

    for (thread = 0; thread < camera_count; thread++)
        pthread_join(threads[thread], NULL);

    return 0;
}
