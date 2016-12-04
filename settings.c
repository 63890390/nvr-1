#include "settings.h"

char *_read_file(char *config_file) {
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

int _jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING &&
        strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, (size_t) (tok->end - tok->start)) == 0) {
        return 0;
    }
    return -1;
}

int _read_json(char *config_file, Settings *settings) {
    char *json = _read_file(config_file);
    jsmn_parser parser;
    jsmntok_t tokens[256];
    int i, c, j;

    jsmn_init(&parser);
    j = jsmn_parse(&parser, json, strlen(json), tokens, 256);

    for (i = 0; i < j; i++)
        if (_jsoneq(json, &tokens[i], "storage_dir") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->storage_dir, json + tokens[i + 1].start, size);
            settings->storage_dir[size] = '\0';
        }

    for (i = 0; i < j; i++)
        if (_jsoneq(json, &tokens[i], "segment_length") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char sls[8];
            memcpy(&sls, json + tokens[i + 1].start, size);
            settings->segment_length = atoi(sls);
        }

    for (c = i = 0; i < j; i++)
        if (_jsoneq(json, &tokens[i], "name") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->cameras[c].name, json + tokens[i + 1].start, size);
            settings->cameras[c].name[size] = '\0';
            c++;
        }


    for (c = i = 0; i < j; i++)
        if (_jsoneq(json, &tokens[i], "uri") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(&settings->cameras[c].uri, json + tokens[i + 1].start, size);
            settings->cameras[c].uri[size] = '\0';
            c++;
        }
    return 0;
}

typedef struct {
    Settings *settings;
    int curr_cam;
} IniHandlerParams;

int _ini_handler(void *user, const char *section, const char *name, const char *value) {
    IniHandlerParams *data = (IniHandlerParams *) user;
    Settings *settings = data->settings;
    if (strcmp(section, "nvr") == 0 && strcmp(name, "storage_dir") == 0) {
        strcpy(settings->storage_dir, value);
    } else if (strcmp(section, "nvr") == 0 && strcmp(name, "segment_length") == 0) {
        settings->segment_length = atoi(value);
    } else if (strcmp(section, "cameras") == 0) {
        strcpy(settings->cameras[data->curr_cam].name, name);
        strcpy(settings->cameras[data->curr_cam].uri, value);
        data->curr_cam++;
    }
    return 0;
}

int _read_ini(char *config_file, Settings *settings) {
    IniHandlerParams data;
    data.settings = settings;
    data.curr_cam = 0;
    ini_parse(config_file, _ini_handler, &data);
    return 0;
}

int read_config(char *config_file, Settings *settings) {
    char *dot = strrchr(config_file, '.');
    if (dot && !strcmp(dot, ".json"))
        return _read_json(config_file, settings);
    else if (dot && !strcmp(dot, ".ini"))
        return _read_ini(config_file, settings);
    else
        return -1;
}