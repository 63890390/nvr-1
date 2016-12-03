#include "settings.h"

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING &&
        strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, (size_t) (tok->end - tok->start)) == 0) {
        return 0;
    }
    return -1;
}

char *read_file(char *config_file) {
    FILE *fp;
    size_t size;
    char *buffer;

    fp = fopen(config_file, "rb");
    if (!fp) {
        perror(config_file);
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    size = (size_t) ftell(fp);
    rewind(fp);

    buffer = calloc(1, size + 1);
    if (!buffer) fclose(fp), fputs("memory alloc fails", stderr), exit(1);

    if (1 != fread(buffer, size, 1, fp))
        fclose(fp), free(buffer), fputs("entire read fails", stderr), exit(1);

    fclose(fp);
    return buffer;
}

int read_config(char *config_file, Settings *settings, Camera cameras[256]) {
    char *dot = strrchr(config_file, '.');
    if (dot && !strcmp(dot, ".json"))
        return read_json(config_file, settings, cameras);
    else if (dot && !strcmp(dot, ".json"))
        return read_ini(config_file, settings, cameras);
    else
        return -1;
}

int read_json(char *config_file, Settings *settings, Camera cameras[256]) {
    char *json = read_file(config_file);
    jsmn_parser parser;
    jsmntok_t tokens[256];
    int i, c, j;

    jsmn_init(&parser);
    j = jsmn_parse(&parser, json, strlen(json), tokens, 256);

    for (i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "storage_dir") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->storage_dir, json + tokens[i + 1].start, size);
            settings->storage_dir[size] = '\0';
        }

    for (i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "segment_length") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char sls[8];
            memcpy(&sls, json + tokens[i + 1].start, size);
            settings->segment_length = atoi(sls);
        }

    for (c = i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "name") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(cameras[c].name, json + tokens[i + 1].start, size);
            cameras[c].name[size] = '\0';
            c++;
        }


    for (c = i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "uri") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(&cameras[c].uri, json + tokens[i + 1].start, size);
            cameras[c].uri[size] = '\0';
            c++;
        }
    return 0;
}

int read_ini(char *config_file, Settings *settings, Camera cameras[256]) {
    printf("ini configuration format is not supported yet\n");
    return -1;
}
