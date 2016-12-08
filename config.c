#include "config.h"

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
    if (j < 0)
        return j;

    for (i = 0; i < j; i++)
        if (_jsoneq(json, &tokens[i], "storage_dir") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->storage_dir, json + tokens[i + 1].start, size);
            settings->storage_dir[size] = '\0';
        } else if (_jsoneq(json, &tokens[i], "segment_length") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char sls[8];
            memcpy(&sls, json + tokens[i + 1].start, size);
            settings->segment_length = atoi(sls);
        } else if (_jsoneq(json, &tokens[i], "retry_delay") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char rdl[8];
            memcpy(&rdl, json + tokens[i + 1].start, size);
            settings->retry_delay = atoi(rdl);
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
    if (strcmp(section, "nvr") == 0) {
        if (strcmp(name, "storage_dir") == 0)
            strcpy(settings->storage_dir, value);
        else if (strcmp(name, "segment_length") == 0)
            settings->segment_length = atoi(value);
        else if (strcmp(name, "retry_delay") == 0)
            settings->retry_delay = atoi(value);
    } else if (strcmp(section, "cameras") == 0) {
        strcpy(settings->cameras[data->curr_cam].name, name);
        strcpy(settings->cameras[data->curr_cam].uri, value);
        data->curr_cam++;
    }
    return 0;
}

int _read_ini(char *config_file, Settings *settings) {
    int r;
    IniHandlerParams data;
    data.settings = settings;
    data.curr_cam = 0;
    if ((r = ini_parse(config_file, _ini_handler, &data)) < 0)
        return r;
    return 0;
}

int read_config(char *config_file, Settings *settings) {
    settings->segment_length = NVR_DEFAULT_SEGMENT_LENGTH;
    settings->retry_delay = NVR_DEFAULT_RETRY_DELAY;
    sprintf(settings->storage_dir, NVR_DEFAULT_STORAGE_DIR);
    char *dot = strrchr(config_file, '.');
    if (dot && !strcmp(dot, ".json"))
        return _read_json(config_file, settings);
    else // if (dot && !strcmp(dot, ".ini"))
        return _read_ini(config_file, settings);
}
