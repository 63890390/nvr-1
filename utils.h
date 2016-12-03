#ifndef NVR_URILS_H
#define NVR_URILS_H

#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <errno.h>

int mkdirs(const char *path, __mode_t mode);

#endif //NVR_URILS_H
