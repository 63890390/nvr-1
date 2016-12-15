#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libavutil/log.h>
#include "ini.h"
#include "jsmn.h"
#include "log.h"

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

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING &&
        strlen(s) == (unsigned int) (tok->end - tok->start) &&
        strncmp(json + tok->start, s, (size_t) (tok->end - tok->start)) == 0) {
        return 0;
    }
    return -1;
}

int get_log_level(const char *name) {
    if (strcasecmp(name, "debug") == 0)
        return NVR_LOG_DEBUG;
    else if (strcasecmp(name, "info") == 0)
        return NVR_LOG_INFO;
    else if (strcasecmp(name, "warning") == 0)
        return NVR_LOG_WARNING;
    else if (strcasecmp(name, "error") == 0)
        return NVR_LOG_ERROR;
    else if (strcasecmp(name, "fatal") == 0)
        return NVR_LOG_FATAL;
    else {
        nvr_log(NVR_LOG_WARNING, "invalid log level: %s", name);
        return get_log_level(NVR_DEFAULT_LOG_LEVEL);
    }
}

int get_ffmpeg_log_level(const char *name) {
    if (strcasecmp(name, "quiet") == 0)
        return AV_LOG_QUIET;
    else if (strcasecmp(name, "debug") == 0)
        return AV_LOG_DEBUG;
    else if (strcasecmp(name, "verbose") == 0)
        return AV_LOG_VERBOSE;
    else if (strcasecmp(name, "info") == 0)
        return AV_LOG_INFO;
    else if (strcasecmp(name, "warning") == 0)
        return AV_LOG_WARNING;
    else if (strcasecmp(name, "error") == 0)
        return AV_LOG_ERROR;
    else if (strcasecmp(name, "fatal") == 0)
        return AV_LOG_FATAL;
    else {
        nvr_log(NVR_LOG_WARNING, "invalid ffmpeg log level: %s", name);
        return get_ffmpeg_log_level(NVR_DEFAULT_FFMPEG_LOG_LEVEL);
    }
}

int read_json(char *config_file, Settings *settings) {
    char *json = read_file(config_file);
    jsmn_parser parser;
    jsmntok_t tokens[256];
    int i, c, j;

    jsmn_init(&parser);
    j = jsmn_parse(&parser, json, strlen(json), tokens, sizeof(tokens));
    if (j < 0)
        return j;

    for (i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "storage_dir") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->storage_dir, json + tokens[i + 1].start, size);
            settings->storage_dir[size] = '\0';
        } else if (jsoneq(json, &tokens[i], "segment_length") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char sls[8];
            memcpy(&sls, json + tokens[i + 1].start, size);
            settings->segment_length = (unsigned int) (atoi(sls) >= 0 ? atoi(sls) : NVR_DEFAULT_SEGMENT_LENGTH);
        } else if (jsoneq(json, &tokens[i], "retry_delay") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char rdl[8];
            memcpy(&rdl, json + tokens[i + 1].start, size);
            settings->retry_delay = (unsigned int) (atoi(rdl) >= 0 ? atoi(rdl) : NVR_DEFAULT_RETRY_DELAY);
        } else if (jsoneq(json, &tokens[i], "conn_timeout") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char cto[8];
            memcpy(&cto, json + tokens[i + 1].start, size);
            settings->conn_timeout = (unsigned int) (atoi(cto) >= 0 ? atoi(cto) : NVR_DEFAULT_CONN_TIMEOUT);
        } else if (jsoneq(json, &tokens[i], "recv_timeout") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            char rto[8];
            memcpy(&rto, json + tokens[i + 1].start, size);
            settings->recv_timeout = (unsigned int) (atoi(rto) >= 0 ? atoi(rto) : NVR_DEFAULT_RECV_TIMEOUT);
        } else if (jsoneq(json, &tokens[i], "log_file") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->log_file, json + tokens[i + 1].start, size);
            settings->log_file[size] = '\0';
        } else if (jsoneq(json, &tokens[i], "log_level") == 0) {
            char level[8];
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(level, json + tokens[i + 1].start, size);
            level[size] = '\0';
            settings->log_level = get_log_level(level);
        } else if (jsoneq(json, &tokens[i], "ffmpeg_log_level") == 0) {
            char level[8];
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(level, json + tokens[i + 1].start, size);
            level[size] = '\0';
            settings->ffmpeg_log_level = get_ffmpeg_log_level(level);
        }

    for (c = i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "name") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(settings->cameras[c].name, json + tokens[i + 1].start, size);
            settings->cameras[c].name[size] = '\0';
            settings->cameras[c].running = 0;
            c++;
        }

    for (c = i = 0; i < j; i++)
        if (jsoneq(json, &tokens[i], "uri") == 0) {
            size_t size = (size_t) (tokens[i + 1].end - tokens[i + 1].start);
            memcpy(&settings->cameras[c].uri, json + tokens[i + 1].start, size);
            settings->cameras[c].uri[size] = '\0';
            c++;
        }

    return 0;
}

struct ini_handler_params {
    Settings *settings;
    unsigned int curr_cam;
};

int ini_parser(void *user, const char *section, const char *name, const char *value) {
    struct ini_handler_params *data = (struct ini_handler_params *) user;
    Settings *settings = data->settings;
    if (strcmp(section, "nvr") == 0) {
        if (strcmp(name, "storage_dir") == 0)
            strncpy(settings->storage_dir, value, sizeof(settings->storage_dir));
        else if (strcmp(name, "segment_length") == 0)
            settings->segment_length = (unsigned int) (atoi(value) >= 0 ? atoi(value) : NVR_DEFAULT_SEGMENT_LENGTH);
        else if (strcmp(name, "retry_delay") == 0)
            settings->retry_delay = (unsigned int) (atoi(value) >= 0 ? atoi(value) : NVR_DEFAULT_RETRY_DELAY);
        else if (strcmp(name, "conn_timeout") == 0)
            settings->conn_timeout = (unsigned int) (atoi(value) >= 0 ? atoi(value) : NVR_DEFAULT_CONN_TIMEOUT);
        else if (strcmp(name, "recv_timeout") == 0)
            settings->recv_timeout = (unsigned int) (atoi(value) >= 0 ? atoi(value) : NVR_DEFAULT_RECV_TIMEOUT);
        else if (strcmp(name, "log_file") == 0)
            strncpy(settings->log_file, value, sizeof(settings->log_file));
        else if (strcmp(name, "log_level") == 0)
            settings->log_level = get_log_level(value);
        else if (strcmp(name, "ffmpeg_log_level") == 0)
            settings->ffmpeg_log_level = get_ffmpeg_log_level(value);
    } else if (strcmp(section, "cameras") == 0) {
        strncpy(settings->cameras[data->curr_cam].name, name, sizeof(settings->cameras[data->curr_cam].name));
        strncpy(settings->cameras[data->curr_cam].uri, value, sizeof(settings->cameras[data->curr_cam].uri));
        settings->cameras[data->curr_cam].running = 0;
        data->curr_cam++;
    }
    return 0;
}

int read_ini(char *config_file, Settings *settings) {
    int r;
    struct ini_handler_params data;
    data.settings = settings;
    data.curr_cam = 0;
    if ((r = ini_parse(config_file, ini_parser, &data)) < 0)
        return r;
    return 0;
}

int read_config(char *config_file, Settings *settings) {
    settings->segment_length = NVR_DEFAULT_SEGMENT_LENGTH;
    settings->retry_delay = NVR_DEFAULT_RETRY_DELAY;
    settings->conn_timeout = NVR_DEFAULT_CONN_TIMEOUT;
    settings->recv_timeout = NVR_DEFAULT_RECV_TIMEOUT;
    strncpy(settings->storage_dir, NVR_DEFAULT_STORAGE_DIR, sizeof(settings->storage_dir));
    strncpy(settings->log_file, NVR_DEFAULT_LOG_FILE, sizeof(settings->log_file));
    settings->log_level = get_log_level(NVR_DEFAULT_LOG_LEVEL);
    settings->ffmpeg_log_level = get_ffmpeg_log_level(NVR_DEFAULT_FFMPEG_LOG_LEVEL);
    char *ext = strrchr(config_file, '.');
    if (ext && !strcmp(ext, ".json"))
        return read_json(config_file, settings);
    else // if (ext && !strcmp(ext, ".ini"))
        return read_ini(config_file, settings);
}
