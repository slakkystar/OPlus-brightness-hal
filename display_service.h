/*
 * display_service.h — OPlus Brightness Adaptor for Transsion (OS15+)
 *
 * Vendor userspace daemon that bridges the ROM's logical brightness /
 * screen-state properties onto the panel's sysfs backlight node.
 * Rewritten in C from the original Rust project by @rianixia — please
 * retain credit if you fork/modify this.
 *
 * This is the OS15+-only build: the legacy OS14 "DisplayPanel" bridge
 * mode has been removed entirely. Only the Curved gamma-2.2 brightness
 * curve is supported.
 *
 * DO NOT DELETE your stock Transsion light HAL — this runs alongside it.
 */
#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <stdbool.h>

/* =======================================================================
 * Constants
 * ===================================================================== */

/* Value written to the backlight node when the screen must be fully off. */
#define OB_BRIGHTNESS_OFF 0

/* Fallback logical (input) brightness range, used when nothing else
 * (persisted range or live IR detection) is available yet. These match
 * persist.sys.oplus.brightness.range.{min,max} defaults. */
#define OB_FALLBACK_MIN 222
#define OB_FALLBACK_MAX 8191

/* Fallback *hardware* (panel) brightness bounds, used only if sysfs
 * detection fails AND ro.vendor.display.oplus.brightness.{max,min}
 * are not set. */
#define OB_DEFAULT_HW_MAX 2047
#define OB_DEFAULT_HW_MIN 8

/* Gamma used for the (only) brightness curve. */
#define OB_GAMMA 2.2f

/* Poll interval for the main loop. Kept short so the easing steps in
 * ob_ease_toward() land often enough to look smooth. */
#define OB_POLL_MS_DEFAULT_MODE 40

#define OB_LOG_TAG "Oplus-DisplayAdaptor"

/* =======================================================================
 * sysfs backlight nodes
 * ===================================================================== */
#define OB_PATH_BRIGHTNESS      "/sys/class/leds/lcd-backlight/brightness"
#define OB_PATH_MIN_BRIGHTNESS  "/sys/class/leds/lcd-backlight/min_brightness"
#define OB_PATH_MAX_BRIGHTNESS  "/sys/class/leds/lcd-backlight/max_hw_brightness"

/* =======================================================================
 * Framework-owned properties (not part of our namespace, read-only to us)
 * ===================================================================== */
#define OB_PROP_SCREEN_BRIGHTNESS "debug.tracing.screen_brightness"
#define OB_PROP_SCREEN_STATE      "debug.tracing.screen_state"

/* Live IR-detected multi-brightness range, written by the ROM itself. */
#define OB_PROP_SYS_MULTIBRIGHTNESS_MAX "sys.oplus.multibrightness"
#define OB_PROP_SYS_MULTIBRIGHTNESS_MIN "sys.oplus.multibrightness.min"

/* =======================================================================
 * RO build/vendor properties — authoritative panel hardware bounds
 * ===================================================================== */
#define OB_PROP_RO_HW_MAX "ro.vendor.display.oplus.brightness.max"
#define OB_PROP_RO_HW_MIN "ro.vendor.display.oplus.brightness.min"

/* =======================================================================
 * Persisted (persist.*) configuration properties
 * ===================================================================== */
#define OB_PROP_DEBUG      "persist.sys.oplus.brightness.debug"
#define OB_PROP_ISFLOAT    "persist.sys.oplus.brightness.isfloat"

/* Kept only for compatibility with older configs / scripts. The adaptor
 * always renders with the Curved gamma-2.2 curve regardless of this
 * value — Linear (1) and Custom (2) modes have been removed. */
#define OB_PROP_MODE       "persist.sys.oplus.brightness.mode"

#define OB_PROP_DISPLAY_TYPE "persist.sys.oplus.brightness.display.type"

/* Manual override of the HW range used for scaling (higher priority than
 * ro.vendor.display.oplus.brightness.*). */
#define OB_PROP_DEVMAX "persist.sys.oplus.brightness.devmax"
#define OB_PROP_DEVMIN "persist.sys.oplus.brightness.devmin"

/* Cache of the detected HW range, written by the service itself. */
#define OB_PROP_HW_MAX "persist.sys.oplus.brightness.hw_max"
#define OB_PROP_HW_MIN "persist.sys.oplus.brightness.hw_min"

/* Cache of the detected logical (input) range, written by the service
 * itself once IR / multibrightness locks it in. */
#define OB_PROP_RANGE_MAX "persist.sys.oplus.brightness.range.max"
#define OB_PROP_RANGE_MIN "persist.sys.oplus.brightness.range.min"

/* Lux AOD (non-panoramic, ambient-driven AOD) fixed brightness. */
#define OB_PROP_LUX_AOD       "persist.sys.oplus.brightness.lux_aod"
#define OB_PROP_LUX_AOD_VALUE "persist.sys.oplus.brightness.lux_aod.value"

/* =======================================================================
 * Types
 * ===================================================================== */

/* Logical (input) brightness range. Seeded from persist.sys.oplus.
 * brightness.range.{min,max} (or OB_FALLBACK_{MIN,MAX}), then "locked"
 * once the ROM's own IR/multi-brightness detection reports a valid
 * range, at which point it's cached back into the persist.* props and
 * never refreshed again for the lifetime of the process. */
typedef struct {
    int min;
    int max;
    bool locked;
} ob_brightness_range_t;

/* =======================================================================
 * Public API
 * ===================================================================== */

/* Entry point: runs the OS15+ brightness poll loop. Never returns. */
void ob_run(void);

#endif /* DISPLAY_SERVICE_H */
