#define _POSIX_C_SOURCE 199309L
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "oplus_brightness/constants.h"
#include "oplus_brightness/logging.h"
#include "oplus_brightness/modes.h"
#include "oplus_brightness/paths.h"
#include "oplus_brightness/properties.h"
#include "oplus_brightness/range.h"
#include "oplus_brightness/scaling.h"
#include "oplus_brightness/state.h"
#include "oplus_brightness/utils.h"
#include "oplus_brightness/writer.h"

static void ob_sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

bool ob_dbg_on(void) {
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

/* -----------------------------------------------------------------------
 * OS15+ brightness poll loop (the only mode this fork supports — the
 * legacy OS14 "DisplayPanel" bridge mode has been removed entirely).
 * --------------------------------------------------------------------- */
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

    int initial = ob_scale_brightness_curved(prev_bright, hw_min, hw_max, range.min, range.max);
    ob_write_brightness(fd, initial, &last_val, dbg);

    bool is_ips = is_ips_mode();
    if (dbg) ob_log_d("[DisplayAdaptor] IPS Mode: %s", is_ips ? "true" : "false");

    for (;;) {
        int cur_state = ob_get_screen_state();
        int raw_bright = ob_get_prop_brightness(&range, is_float);
        int cur_bright;
        if (raw_bright == -1) {
            if (dbg) ob_log_d("[DisplayAdaptor] Brightness is 0, ignoring and keeping previous value.");
            cur_bright = prev_bright;
        } else {
            cur_bright = raw_bright;
        }

        if (cur_bright != prev_bright || cur_state != prev_state) {
            int val_to_write;

            if (cur_state == 2) {
                if (prev_state != 2) {
                    ob_sleep_ms(100);
                }
                val_to_write = ob_scale_brightness_curved(cur_bright, hw_min, hw_max,
                                                            range.min, range.max);
            } else if (is_ips) {
                if (dbg) {
                    ob_log_d("[DisplayAdaptor] IPS Mode: State is %d (OFF), setting brightness 0",
                             cur_state);
                }
                val_to_write = OB_BRIGHTNESS_OFF;
            } else {
                /* AMOLED / default */
                if (cur_state == 0 || cur_state == 1) {
                    if (dbg) ob_log_d("[DisplayAdaptor] State is %d (OFF), setting brightness 0", cur_state);
                    val_to_write = OB_BRIGHTNESS_OFF;
                } else if (cur_state == 3 || cur_state == 4) {
                    bool is_panoramic = ob_is_panoramic_aod_enabled(dbg);

                    if (is_lux_aod && is_panoramic) {
                        int target_lux;
                        if (ob_get_prop_int(OB_PROP_LUX_AOD_VALUE, &target_lux) && target_lux > 0) {
                            if (dbg) {
                                ob_log_d("[DisplayAdaptor] Lux+Panoramic AOD active. Forcing brightness: %d",
                                         target_lux);
                            }
                            val_to_write = target_lux;
                        } else {
                            if (dbg) {
                                ob_log_d("[DisplayAdaptor] Lux+Panoramic AOD active but prop empty/0. "
                                         "Maintaining last value.");
                            }
                            val_to_write = last_val;
                        }
                    } else if (cur_state == 3 && is_lux_aod) {
                        char raw_prop[64] = {0};
                        ob_get_prop(OB_PROP_SCREEN_BRIGHTNESS, raw_prop, sizeof(raw_prop));
                        if (strcmp(raw_prop, "2937.773") == 0) {
                            int lux_val;
                            if (!ob_get_prop_int(OB_PROP_LUX_AOD_VALUE, &lux_val)) lux_val = 1;
                            if (dbg) {
                                ob_log_d("[DisplayAdaptor] Lux AOD: Detected, forcing brightness to %d", lux_val);
                            }
                            val_to_write = lux_val;
                        } else {
                            if (dbg) {
                                ob_log_d("[DisplayAdaptor] State is 3 (Doze) & Lux AOD ON: Updating brightness: %d",
                                         cur_bright);
                            }
                            val_to_write = ob_scale_brightness_curved(cur_bright, hw_min, hw_max,
                                                                        range.min, range.max);
                        }
                    } else if (is_panoramic) {
                        if (dbg) {
                            ob_log_d("[DisplayAdaptor] State is %d Panoramic AOD is ON, skipping brightness write",
                                     cur_state);
                        }
                        val_to_write = last_val;
                    } else {
                        if (dbg) {
                            ob_log_d("[DisplayAdaptor] State is %d Panoramic AOD is OFF, setting brightness 0",
                                     cur_state);
                        }
                        val_to_write = OB_BRIGHTNESS_OFF;
                    }
                } else if (prev_state == 2) {
                    if (ob_is_panoramic_aod_enabled(dbg)) {
                        if (dbg) {
                            ob_log_d("[DisplayAdaptor] Transitioned from ON with Panoramic AOD, "
                                     "deferring brightness 0");
                        }
                        val_to_write = last_val;
                    } else {
                        if (dbg) {
                            ob_log_d("[DisplayAdaptor] Transitioned from ON without Panoramic AOD, "
                                     "setting brightness 0");
                        }
                        val_to_write = OB_BRIGHTNESS_OFF;
                    }
                } else {
                    val_to_write = last_val;
                }
            }

            if (val_to_write != last_val) {
                ob_write_brightness(fd, val_to_write, &last_val, dbg);
            }
        }

        prev_bright = cur_bright;
        prev_state = cur_state;
        ob_sleep_ms(OB_POLL_MS_DEFAULT_MODE);
    }
}
