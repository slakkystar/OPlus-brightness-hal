/*
 * display_service.c — OPlus Brightness Adaptor for Transsion (OS15+)
 * See display_service.h for the overview.
 */
#define _POSIX_C_SOURCE 200809L

#include <android/log.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "display_service.h"

/* __system_property_set() is not declared in the public NDK headers,
 * but is still exported by libc.so on-device (and by the NDK's stub
 * libc.so at link time), so we declare it ourselves — this avoids a
 * dependency on libcutils / a full AOSP source tree, so this builds
 * fine with a plain NDK. */
extern int __system_property_set(const char *key, const char *value);

/* =======================================================================
 * Logging
 * ===================================================================== */

static void ob_log_d(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    __android_log_print(ANDROID_LOG_DEBUG, OB_LOG_TAG, "%s", buf);
}

static void ob_log_e(const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    __android_log_print(ANDROID_LOG_ERROR, OB_LOG_TAG, "%s", buf);
}

/* =======================================================================
 * System properties
 * ===================================================================== */

static bool ob_get_prop(const char *key, char *out, size_t out_size) {
    char buf[PROP_VALUE_MAX];
    int len = __system_property_get(key, buf);
    if (len <= 0 || out_size == 0) {
        return false;
    }
    strncpy(out, buf, out_size - 1);
    out[out_size - 1] = '\0';
    return true;
}

static bool ob_get_prop_int(const char *key, int *out) {
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

static bool ob_get_prop_bool(const char *key) {
    char buf[PROP_VALUE_MAX];
    int len = __system_property_get(key, buf);
    return len > 0 && strcmp(buf, "true") == 0;
}

static bool ob_set_prop(const char *key, const char *val) {
    return __system_property_set(key, val) == 0;
}

static bool ob_set_prop_int(const char *key, int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    return ob_set_prop(key, buf);
}

/* =======================================================================
 * Brightness curve — Curved gamma-2.2 only (Linear/Custom removed)
 * ===================================================================== */

static int ob_scale_brightness_curved(int val, int hw_min, int hw_max,
                                       int input_min, int input_max) {
    if (val <= input_min) {
        return hw_min;
    }
    if (val >= input_max) {
        return hw_max;
    }

    float range_input = (float)(input_max - input_min);
    float range_hw = (float)(hw_max - hw_min);
    float ratio = (float)(val - input_min) / range_input;
    float curve = powf(ratio, OB_GAMMA);

    return (int)lroundf((float)hw_min + curve * range_hw);
}

/* Eases `*current` one step closer to `target` (quarter of the remaining
 * distance per call, minimum step of 1 unit). This is what makes
 * brightness changes look smooth instead of instantly jumping. */
static void ob_ease_toward(int *current, int target) {
    if (*current == target) {
        return;
    }
    int diff = target - *current;
    int step = diff / 4;
    if (step == 0) {
        step = (diff > 0) ? 1 : -1;
    }
    *current += step;
    if ((step > 0 && *current > target) || (step < 0 && *current < target)) {
        *current = target;
    }
}

/* =======================================================================
 * Logical brightness range (persisted / IR-locked)
 * ===================================================================== */

static void ob_range_init(ob_brightness_range_t *range, bool dbg) {
    int pmin, pmax;
    bool have_min = ob_get_prop_int(OB_PROP_RANGE_MIN, &pmin);
    bool have_max = ob_get_prop_int(OB_PROP_RANGE_MAX, &pmax);

    if (have_min && have_max && pmin < pmax) {
        range->min = pmin;
        range->max = pmax;
    } else {
        range->min = OB_FALLBACK_MIN;
        range->max = OB_FALLBACK_MAX;
    }
    range->locked = false;

    if (dbg) {
        ob_log_d("[BrightnessRange] Initialized with range: min=%d, max=%d",
                 range->min, range->max);
    }
}

static void ob_range_refresh(ob_brightness_range_t *range, bool dbg) {
    if (range->locked) {
        return;
    }

    int pmin, pmax, rmin, rmax;
    bool have_pmin = ob_get_prop_int(OB_PROP_RANGE_MIN, &pmin);
    bool have_pmax = ob_get_prop_int(OB_PROP_RANGE_MAX, &pmax);
    bool have_rmin = ob_get_prop_int(OB_PROP_SYS_MULTIBRIGHTNESS_MIN, &rmin);
    bool have_rmax = ob_get_prop_int(OB_PROP_SYS_MULTIBRIGHTNESS_MAX, &rmax);

    if (have_rmin && have_rmax && rmin < rmax) {
        range->min = rmin;
        range->max = rmax;
        if (!(have_pmin && pmin == rmin)) {
            ob_set_prop_int(OB_PROP_RANGE_MIN, rmin);
        }
        if (!(have_pmax && pmax == rmax)) {
            ob_set_prop_int(OB_PROP_RANGE_MAX, rmax);
        }
        range->locked = true;
        if (dbg) {
            ob_log_d("[BrightnessRange] Locked from live IR detection: min=%d, max=%d",
                     range->min, range->max);
        }
    } else if (have_pmin && have_pmax && pmin < pmax) {
        range->min = pmin;
        range->max = pmax;
    } else {
        range->min = OB_FALLBACK_MIN;
        range->max = OB_FALLBACK_MAX;
    }

    if (range->min >= range->max) {
        range->min = OB_FALLBACK_MIN;
        range->max = OB_FALLBACK_MAX;
    }
}

/* =======================================================================
 * Framework state (logical brightness / screen state)
 * ===================================================================== */

static int ob_get_prop_brightness(const ob_brightness_range_t *range, bool is_float) {
    char buf[64];
    if (!ob_get_prop(OB_PROP_SCREEN_BRIGHTNESS, buf, sizeof(buf))) {
        return OB_FALLBACK_MIN;
    }

    if (is_float) {
        char *end = NULL;
        float f = strtof(buf, &end);
        if (end == buf) {
            return OB_FALLBACK_MIN;
        }
        if (f == 0.0f) {
            return -1; /* skip write, keep previous */
        }
        if (f < 0.0f) f = 0.0f;
        if (f > 1.0f) f = 1.0f;
        return (int)lroundf((float)range->min + f * (float)(range->max - range->min));
    }

    /* Take only the leading integer portion, e.g. "2418.0" -> 2418. */
    char intbuf[32];
    size_t n = 0;
    for (size_t i = 0; buf[i] != '\0' && buf[i] != '.' && n < sizeof(intbuf) - 1; i++) {
        intbuf[n++] = buf[i];
    }
    intbuf[n] = '\0';
    if (n == 0) {
        return OB_FALLBACK_MIN;
    }

    char *end = NULL;
    long v = strtol(intbuf, &end, 10);
    if (end == intbuf) {
        return OB_FALLBACK_MIN;
    }
    if (v == 0) {
        return -1; /* skip write, keep previous */
    }
    return (int)v;
}

static int ob_get_screen_state(void) {
    int v;
    if (ob_get_prop_int(OB_PROP_SCREEN_STATE, &v)) {
        return v;
    }
    return 2; /* default to ON */
}

/* =======================================================================
 * Misc helpers: sysfs int reads, HW brightness bounds, Panoramic AOD
 * ===================================================================== */

static bool ob_read_file_int(const char *path, int *out) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }

    char buf[64];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    char *p = buf;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;

    char digits[32];
    size_t d = 0;
    while (isdigit((unsigned char)*p) && d < sizeof(digits) - 1) {
        digits[d++] = *p++;
    }
    digits[d] = '\0';

    if (d == 0) {
        return false;
    }
    *out = (int)strtol(digits, NULL, 10);
    return true;
}

static long ob_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* Runs `settings get secure panoramic_aod_enable` and returns true iff
 * the result is "1". Cached for CACHE_MS so we don't spawn a shell on
 * every ~40ms poll while the screen is in AOD. */
static bool ob_is_panoramic_aod_enabled(bool dbg) {
    static bool cached = false;
    static long cached_at_ms = 0;
    const long CACHE_MS = 1500;

    long now = ob_now_ms();
    if (cached_at_ms != 0 && (now - cached_at_ms) < CACHE_MS) {
        return cached;
    }

    FILE *p = popen("settings get secure panoramic_aod_enable 2>/dev/null", "r");
    if (!p) {
        if (dbg) {
            ob_log_e("[DisplayAdaptor] Failed to execute 'settings' command");
        }
        cached = false;
        cached_at_ms = now;
        return cached;
    }

    char buf[32] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, p);
    pclose(p);
    buf[n] = '\0';

    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
        buf[--n] = '\0';
    }

    if (dbg) {
        ob_log_d("[DisplayAdaptor] panoramic_aod_enable result: '%s'", buf);
    }

    cached = (strcmp(buf, "1") == 0);
    cached_at_ms = now;
    return cached;
}

/* Detects the panel's hardware max brightness, in priority order:
 * devmax override -> cached hw_max -> live sysfs read (cached) ->
 * ro.vendor.display.oplus.brightness.max -> OB_DEFAULT_HW_MAX. */
static int ob_get_max_brightness(bool dbg) {
    int custom_max;
    if (ob_get_prop_int(OB_PROP_DEVMAX, &custom_max) && custom_max > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Using custom devmax brightness: %d", custom_max);
        return custom_max;
    }

    int cached_max;
    if (ob_get_prop_int(OB_PROP_HW_MAX, &cached_max)) {
        if (dbg) ob_log_d("[DisplayAdaptor] Using cached hw_max: %d", cached_max);
        return cached_max;
    }

    int val;
    if (ob_read_file_int(OB_PATH_MAX_BRIGHTNESS, &val)) {
        if (dbg) ob_log_d("[DisplayAdaptor] Detected hw_max: %d. Saving to prop.", val);
        ob_set_prop_int(OB_PROP_HW_MAX, val);
        return val;
    }

    int ro_max;
    if (ob_get_prop_int(OB_PROP_RO_HW_MAX, &ro_max) && ro_max > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_max, using ro prop: %d", ro_max);
        return ro_max;
    }

    if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_max, using default %d", OB_DEFAULT_HW_MAX);
    return OB_DEFAULT_HW_MAX;
}

/* Same priority order as ob_get_max_brightness(), for the min bound. */
static int ob_get_min_brightness(bool dbg) {
    int custom_min;
    if (ob_get_prop_int(OB_PROP_DEVMIN, &custom_min) && custom_min > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Using custom devmin brightness: %d", custom_min);
        return custom_min;
    }

    int cached_min;
    if (ob_get_prop_int(OB_PROP_HW_MIN, &cached_min)) {
        if (cached_min > 0) {
            if (dbg) ob_log_d("[DisplayAdaptor] Using cached hw_min: %d", cached_min);
            return cached_min;
        }
        if (dbg) ob_log_d("[DisplayAdaptor] Cached hw_min invalid (%d), forcing to %d.",
                           cached_min, OB_DEFAULT_HW_MIN);
        ob_set_prop_int(OB_PROP_HW_MIN, OB_DEFAULT_HW_MIN);
        return OB_DEFAULT_HW_MIN;
    }

    int val;
    if (ob_read_file_int(OB_PATH_MIN_BRIGHTNESS, &val)) {
        if (val <= 0) {
            if (dbg) ob_log_d("[DisplayAdaptor] Detected hw_min <= 0 (screen off?), falling back to %d.",
                               OB_DEFAULT_HW_MIN);
            val = OB_DEFAULT_HW_MIN;
        }
        if (dbg) ob_log_d("[DisplayAdaptor] Saving hw_min: %d to prop.", val);
        ob_set_prop_int(OB_PROP_HW_MIN, val);
        return val;
    }

    int ro_min;
    if (ob_get_prop_int(OB_PROP_RO_HW_MIN, &ro_min) && ro_min > 0) {
        if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_min, using ro prop: %d", ro_min);
        return ro_min;
    }

    if (dbg) ob_log_d("[DisplayAdaptor] Failed to detect hw_min, using default %d", OB_DEFAULT_HW_MIN);
    return OB_DEFAULT_HW_MIN;
}

/* =======================================================================
 * Writer
 * ===================================================================== */

static void ob_write_brightness(int fd, int val, int *last_val, bool dbg) {
    if (*last_val == val) {
        return;
    }
    if (dbg) {
        ob_log_d("[DisplayAdaptor] Writing brightness: %d -> %d", *last_val, val);
    }

    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", val);
    if (len < 0) {
        return;
    }

    ssize_t written = write(fd, buf, (size_t)len);
    if (written < 0) {
        if (dbg) {
            ob_log_e("[DisplayAdaptor] Write failed for value %d", val);
        }
    } else {
        *last_val = val;
    }
}

/* =======================================================================
 * Config helpers
 * ===================================================================== */

static bool ob_dbg_on(void) {
    return ob_get_prop_bool(OB_PROP_DEBUG);
}

static bool is_float_mode(void) {
    return ob_get_prop_bool(OB_PROP_ISFLOAT);
}

static bool is_ips_mode(void) {
    char buf[32];
    return ob_get_prop(OB_PROP_DISPLAY_TYPE, buf, sizeof(buf)) && strcmp(buf, "IPS") == 0;
}

static bool is_lux_aod_mode(void) {
    return ob_get_prop_bool(OB_PROP_LUX_AOD);
}

/* Only Curved gamma-2.2 is supported. If a legacy config still sets
 * persist.sys.oplus.brightness.mode to 1 (Linear) or 2 (Custom), warn
 * once in the debug log and ignore it — we always render Curved. */
static void warn_if_legacy_mode_set(bool dbg) {
    int mode;
    if (dbg && ob_get_prop_int(OB_PROP_MODE, &mode) && mode != 0) {
        ob_log_d("[DisplayAdaptor] %s=%d is ignored: only Curved gamma-2.2 is supported now.",
                 OB_PROP_MODE, mode);
    }
}

static void ob_sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* =======================================================================
 * Main poll loop
 * ===================================================================== */

void ob_run(void) {
    bool dbg = ob_dbg_on();
    if (dbg) ob_log_d("[DisplayAdaptor] Starting...");
    warn_if_legacy_mode_set(dbg);

    bool is_float = is_float_mode();
    bool is_lux_aod = is_lux_aod_mode();

    if (dbg) {
        ob_log_d("[DisplayAdaptor] Curve: Gamma 2.2 (Curved), Lux AOD: %s",
                 is_lux_aod ? "true" : "false");
    }

    int hw_min = ob_get_min_brightness(dbg);
    int hw_max = ob_get_max_brightness(dbg);

    ob_brightness_range_t range;
    ob_range_init(&range, dbg);
    ob_range_refresh(&range, dbg);
    if (dbg) ob_log_d("[DisplayAdaptor] IR locked: min=%d, max=%d", range.min, range.max);

    int fd = open(OB_PATH_BRIGHTNESS, O_WRONLY);
    if (fd < 0) {
        ob_log_e("[DisplayAdaptor] Could not open brightness file");
        return;
    }

    int last_val = -1;
    int prev_state = ob_get_screen_state();
    int prev_bright = ob_get_prop_brightness(&range, is_float);
    if (prev_bright == -1) {
        if (dbg) ob_log_d("[DisplayAdaptor] Initial brightness is 0, using fallback.");
        prev_bright = OB_FALLBACK_MIN;
    }

    /* hw_now is the actual, eased hardware value we write. It chases
     * hw_target rather than jumping straight to it. */
    int hw_now = ob_scale_brightness_curved(prev_bright, hw_min, hw_max, range.min, range.max);
    ob_write_brightness(fd, hw_now, &last_val, dbg);

    bool is_ips = is_ips_mode();
    if (dbg) ob_log_d("[DisplayAdaptor] IPS Mode: %s", is_ips ? "true" : "false");

    for (;;) {
        int cur_state = ob_get_screen_state();
        int raw_bright = ob_get_prop_brightness(&range, is_float);
        int cur_bright = (raw_bright == -1) ? prev_bright : raw_bright;

        int hw_target;
        bool snap_immediately = false; /* true = write instantly, no easing */

        if (cur_state == 2) {
            if (prev_state != 2) {
                ob_sleep_ms(100);
            }
            hw_target = ob_scale_brightness_curved(cur_bright, hw_min, hw_max,
                                                     range.min, range.max);
        } else if (is_ips) {
            if (cur_state != prev_state && dbg) {
                ob_log_d("[DisplayAdaptor] IPS Mode: State is %d (OFF), setting brightness 0",
                         cur_state);
            }
            hw_target = OB_BRIGHTNESS_OFF;
            snap_immediately = true;
        } else {
            /* AMOLED / default */
            if (cur_state == 0 || cur_state == 1) {
                if (cur_state != prev_state && dbg) {
                    ob_log_d("[DisplayAdaptor] State is %d (OFF), setting brightness 0", cur_state);
                }
                hw_target = OB_BRIGHTNESS_OFF;
                snap_immediately = true;
            } else if (cur_state == 3 || cur_state == 4) {
                bool is_panoramic = ob_is_panoramic_aod_enabled(dbg);

                if (is_panoramic) {
                    /* Full-screen (Panoramic) AOD: the ROM itself already
                     * animates dozeScreenBrightness based on the ambient
                     * light sensor. Just follow that value like we do
                     * on-screen — don't clamp to a fixed lux_aod value,
                     * or we fight the ROM's own animation. */
                    if (cur_state != prev_state && dbg) {
                        ob_log_d("[DisplayAdaptor] Panoramic AOD active, following ROM brightness: %d",
                                 cur_bright);
                    }
                    hw_target = ob_scale_brightness_curved(cur_bright, hw_min, hw_max,
                                                             range.min, range.max);
                } else if (is_lux_aod) {
                    /* Simple (non-panoramic) AOD: there's no continuous
                     * brightness ramp from the ROM to follow, so pin to
                     * the configured fixed HW brightness instead. */
                    int target_lux;
                    if (!ob_get_prop_int(OB_PROP_LUX_AOD_VALUE, &target_lux) || target_lux <= 0) {
                        target_lux = hw_min;
                    }
                    if (cur_state != prev_state && dbg) {
                        ob_log_d("[DisplayAdaptor] Lux AOD (non-panoramic) active, fixed brightness: %d",
                                 target_lux);
                    }
                    hw_target = target_lux;
                } else {
                    if (cur_state != prev_state && dbg) {
                        ob_log_d("[DisplayAdaptor] State is %d, AOD off, setting brightness 0",
                                 cur_state);
                    }
                    hw_target = OB_BRIGHTNESS_OFF;
                    snap_immediately = true;
                }
            } else if (prev_state == 2) {
                if (ob_is_panoramic_aod_enabled(dbg)) {
                    if (dbg) {
                        ob_log_d("[DisplayAdaptor] Transitioned from ON with Panoramic AOD, "
                                 "deferring brightness 0");
                    }
                    hw_target = hw_now;
                } else {
                    if (dbg) {
                        ob_log_d("[DisplayAdaptor] Transitioned from ON without Panoramic AOD, "
                                 "setting brightness 0");
                    }
                    hw_target = OB_BRIGHTNESS_OFF;
                    snap_immediately = true;
                }
            } else {
                hw_target = hw_now;
            }
        }

        if (snap_immediately) {
            hw_now = hw_target;
        } else {
            ob_ease_toward(&hw_now, hw_target);
        }

        if (hw_now != last_val) {
            ob_write_brightness(fd, hw_now, &last_val, dbg);
        }

        prev_bright = cur_bright;
        prev_state = cur_state;
        ob_sleep_ms(OB_POLL_MS_DEFAULT_MODE);
    }
}

/* =======================================================================
 * Entry point
 * ===================================================================== */

int main(void) {
    ob_run();
    return 0;
}
