/*
 * OPlus Brightness Adaptor for Transsion (MediaTek ColorOS ports)
 * constants.h — global constants
 *
 * Rewritten in C from the original Rust implementation.
 * Author of original logic: rianixia. Please retain credit if you
 * fork/redistribute this HAL.
 */
#ifndef OPLUS_BRIGHTNESS_CONSTANTS_H
#define OPLUS_BRIGHTNESS_CONSTANTS_H

/* Value written to the backlight node when the screen must be fully off. */
#define OB_BRIGHTNESS_OFF 0

/* Fallback logical (input) brightness range, used when nothing else
 * (persisted range or live IR detection) is available yet. These match
 * persist.sys.oplus.brightness.range.{min,max} defaults. */
#define OB_FALLBACK_MIN 222
#define OB_FALLBACK_MAX 8191

/* Fallback *hardware* (panel) brightness bounds, used only if sysfs
 * detection fails AND ro.vendor.display.oplus.brightness.{max,min}
 * are not set. These mirror the example values from the vendor prop
 * documentation (2047 / 8). */
#define OB_DEFAULT_HW_MAX 2047
#define OB_DEFAULT_HW_MIN 8

/* Gamma used for the (only) brightness curve. Always Curved gamma 2.2 —
 * this project intentionally does not support Linear/Custom curves. */
#define OB_GAMMA 2.2f

/* Poll interval for the main loop. */
#define OB_POLL_MS_DEFAULT_MODE 100

#define OB_LOG_TAG "Oplus-DisplayAdaptor"

#endif /* OPLUS_BRIGHTNESS_CONSTANTS_H */
