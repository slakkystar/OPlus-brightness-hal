#include <android/log.h>
#include <stdarg.h>
#include <stdio.h>

#include "oplus_brightness/constants.h"
#include "oplus_brightness/logging.h"

void ob_log_d(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    __android_log_print(ANDROID_LOG_DEBUG, OB_LOG_TAG, "%s", buf);
}

void ob_log_e(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    __android_log_print(ANDROID_LOG_ERROR, OB_LOG_TAG, "%s", buf);
}
