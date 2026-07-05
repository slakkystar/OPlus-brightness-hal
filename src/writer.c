#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "oplus_brightness/logging.h"
#include "oplus_brightness/writer.h"

void ob_write_brightness(int fd, int val, int *last_val, bool dbg) {
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
            ob_log_e("[DisplayAdaptor] Write failed for value %d: %s", val, strerror(errno));
        }
    } else {
        *last_val = val;
    }
}
