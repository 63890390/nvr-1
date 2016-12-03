#include "utils.h"

int mkdirs(const char *path, __mode_t mode) {
    const size_t len = strlen(path);
    char cur_path[PATH_MAX];
    char *p;
    errno = 0;
    if (len > sizeof(cur_path) - 1) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(cur_path, path);

    for (p = cur_path + 1; *p; p++)
        if (*p == '/') {
            *p = '\0';
            if (mkdir(cur_path, mode) != 0)
                if (errno != EEXIST)
                    return -1;
            *p = '/';
        }

    if (mkdir(cur_path, mode) != 0)
        if (errno != EEXIST)
            return -1;

    return 0;
}
