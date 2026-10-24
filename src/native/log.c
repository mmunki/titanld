#include "titangl.h"

#include <stdarg.h>
#include <stdio.h>

static int to_android_level(int level)
{
    switch (level) {
    case TITANGL_ERROR:
        return ANDROID_LOG_ERROR;
    case TITANGL_WARN:
        return ANDROID_LOG_WARN;
    case TITANGL_DEBUG:
        return ANDROID_LOG_DEBUG;
    case TITANGL_INFO:
    default:
        return ANDROID_LOG_INFO;
    }
}

void titangl_log(int level, const char *fmt, ...)
{
    if (level > titangl_config_state.log_level) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(to_android_level(level), TITANGL_LOG_TAG, fmt, ap);
    va_end(ap);
}
