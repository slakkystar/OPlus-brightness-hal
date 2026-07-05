#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>

#include "oplus_brightness/properties.h"

/* __system_property_set() is not declared in the public NDK headers,
 * but is still exported by libc.so on-device (and by the NDK's stub
 * libc.so at link time), so we declare it ourselves — this mirrors
 * what the original Rust implementation did via an `unsafe extern`
 * block, and lets this file build with a plain NDK (no full AOSP
 * source tree / libcutils required). */
extern int __system_property_set(const char *key, const char *value);

bool ob_get_prop(const char *key, char *out, size_t out_size) {
    char buf[PROP_VALUE_MAX];
    int len = __system_property_get(key, buf);
    if (len <= 0 || out_size == 0) {
        return false;
    }
    strncpy(out, buf, out_size - 1);
    out[out_size - 1] = '\0';
    return true;
}

bool ob_get_prop_int(const char *key, int *out) {
    char buf[PROP_VALUE_MAX];
    int len = __system_property_get(key, buf);
    if (len <= 0) {
        return false;
    }
    char *end = NULL;
    long v = strtol(buf, &end, 10);
    if (end == buf) {
        return false;
    }
    *out = (int)v;
    return true;
}

bool ob_get_prop_bool(const char *key) {
    char buf[PROP_VALUE_MAX];
    int len = __system_property_get(key, buf);
    return len > 0 && strcmp(buf, "true") == 0;
}

bool ob_set_prop(const char *key, const char *val) {
    return __system_property_set(key, val) == 0;
}

bool ob_set_prop_int(const char *key, int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    return ob_set_prop(key, buf);
}
