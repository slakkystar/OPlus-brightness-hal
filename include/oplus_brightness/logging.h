#ifndef OPLUS_BRIGHTNESS_LOGGING_H
#define OPLUS_BRIGHTNESS_LOGGING_H

/* Thin wrappers around liblog. Callers gate these behind their own
 * `dbg` boolean (read once per run from OB_PROP_DEBUG), mirroring the
 * original Rust `if dbg { log_d(...) }` style. */
void ob_log_d(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void ob_log_e(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* OPLUS_BRIGHTNESS_LOGGING_H */
