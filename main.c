#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "nvr.h"

Settings settings;
Camera cameras[256];

void main_shutdown(int sig) {
    (void) (sig);
    printf("\rshutting down\n");
    settings.running = 0;
}

void read_configuration(int sig) {
    (void) (sig);
    read_config("config.json", &settings, cameras);
    settings.running = 1;
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
    signal(SIGHUP, read_configuration);

    int thread, camera_count = 0;
    jsmn_parser parser;
    jsmn_init(&parser);
    pthread_t threads[256];
    read_configuration(0);

    while (strlen(cameras[camera_count].name) > 0) {
        printf("camera \"%s\" \"%s\"\n", cameras[camera_count].name, cameras[camera_count].uri);
        pthread_create(&threads[camera_count], NULL, record_thread, &cameras[camera_count]);
        camera_count++;
    }

    for (thread = 0; thread < camera_count; thread++)
        pthread_join(threads[thread], NULL);

    return 0;
}
