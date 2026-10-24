/*
 * TitanGL configuration reader.
 *
 * The file is a small JSON object written by the TitanLD manager app:
 *
 *   {
 *     "glVersion": "4.6",
 *     "maxGlslCacheSize": 64,
 *     "logLevel": 3,
 *     "forceGLESVersion": 0,
 *     "dropDesktopExtensions": true
 *   }
 *
 * Only the keys the renderer understands are read.  A missing or malformed
 * file is not an error: the built-in defaults are used.
 */
#include "titangl.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

titangl_config titangl_config_state = {
    4,   /* fake_major */
    6,   /* fake_minor */
    64,  /* max_glsl_cache_size (MiB) */
    TITANGL_INFO,
    0,   /* force_es_version: 0 = automatic, else 2 or 3 */
    1,   /* drop_desktop_extensions */
    {0},
};

static int read_text(const char *path, char *out, size_t cap)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    ssize_t n = read(fd, out, cap - 1);
    close(fd);
    if (n < 0) {
        return -1;
    }
    out[n] = '\0';
    return (int)n;
}

static const char *find_value(const char *json, const char *key)
{
    char pattern[96];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) {
        return NULL;
    }
    p += strlen(pattern);
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p != ':') {
        return NULL;
    }
    p++;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

static int read_int(const char *json, const char *key, int fallback)
{
    const char *v = find_value(json, key);
    if (!v) {
        return fallback;
    }
    char *end = NULL;
    long value = strtol(v, &end, 10);
    if (end == v) {
        return fallback;
    }
    return (int)value;
}

static int read_bool(const char *json, const char *key, int fallback)
{
    const char *v = find_value(json, key);
    if (!v) {
        return fallback;
    }
    if (strncmp(v, "true", 4) == 0 || *v == '1') {
        return 1;
    }
    if (strncmp(v, "false", 5) == 0 || *v == '0') {
        return 0;
    }
    return fallback;
}

static void read_gl_version(const char *json)
{
    const char *v = find_value(json, "glVersion");
    if (!v || *v != '"') {
        return;
    }
    v++;
    int major = 0;
    while (isdigit((unsigned char)*v)) {
        major = major * 10 + (*v - '0');
        v++;
    }
    int minor = 0;
    if (*v == '.') {
        v++;
        while (isdigit((unsigned char)*v)) {
            minor = minor * 10 + (*v - '0');
            v++;
        }
    }
    if (major >= 2 && major <= 9 && minor >= 0 && minor <= 9) {
        titangl_config_state.fake_major = major;
        titangl_config_state.fake_minor = minor;
    }
}

static int file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

void titangl_config_load(void)
{
    static const char *candidates[] = {
        "/sdcard/TitanLD/config.json",
        "/storage/emulated/0/TitanLD/config.json",
        "/sdcard/MG/config.json",
        "/storage/emulated/0/MG/config.json",
    };
    char json[65536];
    const char *env_path = getenv("TITANLD_CONFIG");
    const char *path = NULL;
    size_t i;

    if (env_path && file_exists(env_path)) {
        path = env_path;
    }
    for (i = 0; !path && i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (file_exists(candidates[i])) {
            path = candidates[i];
            break;
        }
    }
    if (!path) {
        path = candidates[0];
    }
    snprintf(titangl_config_state.config_path,
             sizeof(titangl_config_state.config_path), "%s", path);

    if (!file_exists(path) || read_text(path, json, sizeof(json)) < 0) {
        TLOG(TITANGL_DEBUG, "no config at %s, using defaults", path);
        return;
    }

    read_gl_version(json);
    titangl_config_state.max_glsl_cache_size =
        read_int(json, "maxGlslCacheSize", titangl_config_state.max_glsl_cache_size);
    titangl_config_state.log_level =
        read_int(json, "logLevel", titangl_config_state.log_level);
    titangl_config_state.force_es_version =
        read_int(json, "forceGLESVersion", titangl_config_state.force_es_version);
    titangl_config_state.drop_desktop_extensions =
        read_bool(json, "dropDesktopExtensions",
                  titangl_config_state.drop_desktop_extensions);

    TLOG(TITANGL_INFO, "config: %s (fake GL %d.%d, log level %d)",
         path, titangl_config_state.fake_major, titangl_config_state.fake_minor,
         titangl_config_state.log_level);
}
