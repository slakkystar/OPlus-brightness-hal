# OPlus Brightness Adaptor for Transsion — C rewrite (OS15+ only)

A vendor userspace daemon for OPlus/Oppo/Realme (ColorOS) ports on
Transsion (MediaTek) devices. It bridges the ROM's logical brightness /
screen-state properties onto the panel's sysfs backlight node.

This is a rewrite, in C, of the original Rust project by **@rianixia**.
This fork targets **OS15 and above only** — the original project's
legacy OS14 "DisplayPanel" bridge mode has been removed entirely (no
`/data/addon/oplus_display` bridge file, no panel-mode dispatch, no
`persist.sys.oplus.brightness.panelmode/panel.min/panel.max` props).
If you need OS14 support, use the OS14 branch/build instead.

Behaviour otherwise matches the original except:

* Only the **Curved gamma-2.2** brightness curve is implemented — the
  original's Linear and Custom (75%→255) modes have been removed, since
  Curved is always the smoothest and is what this fork wants.
* All configuration properties were moved from the `rianixia` namespace
  to an `oplus.brightness` namespace (see table below).
* `ro.vendor.display.oplus.brightness.{max,min}` is now consulted as a
  build-time-known fallback for the panel's real hardware bounds if
  live sysfs detection fails.

**DO NOT DELETE your stock Transsion light HAL.** This runs alongside it.

## Building

### Option A — plain Android NDK (no AOSP tree required)

See `.github/workflows/build.yml`: it cross-compiles directly with the
NDK's clang (`aarch64-linux-android21-clang`), linking only against
`liblog` — no `libcutils`/full source tree needed. Grab the resulting
binary + `.rc` from the workflow's uploaded artifact.

### Option B — full vendor/AOSP source tree

Drop this directory into your vendor tree (e.g.
`vendor/oplus_brightness_adaptor/`) and build the module with Soong:

```
mmm vendor/oplus_brightness_adaptor
```

Either way you end up with:

```
/vendor/bin/hw/vendor.oplus.brightness.adaptor
/vendor/etc/init/init.oplus.brightness.adaptor.rc
```

### SEPolicy

None is included. Either run in permissive mode for this domain or add
a dedicated sepolicy label — each device's sysfs node labels differ.
Reusing the stock light HAL's domain (often `mtk_hal_light_exec`) is a
reasonable shortcut; see the commented `seclabel` line in the `.rc`.

## Properties

### RO (set in vendor.prop / board build.prop)

| Property | Example | Meaning |
|---|---|---|
| `ro.vendor.display.oplus.brightness.max` | `2047` | Panel's true max HW brightness, used if sysfs detection fails |
| `ro.vendor.display.oplus.brightness.min` | `8` | Panel's true min HW brightness, used if sysfs detection fails |

### Persist (persist.prop; also mirror into vendor.prop if you want them to survive `fastboot format`)

| Property | Default | Meaning |
|---|---|---|
| `persist.sys.oplus.brightness.debug` | `false` | `true` enables verbose logcat (tag `Oplus-DisplayAdaptor`) |
| `persist.sys.oplus.brightness.isfloat` | `false` | `false`: `debug.tracing.screen_brightness` is an int like `2418.0` (leading int part used). `true`: it's a float `0.0–1.0`, mapped onto the input range. |
| `persist.sys.oplus.brightness.mode` | `0` | Kept for compatibility only. **Always renders Curved gamma-2.2** regardless of value — Linear (1) / Custom (2) no longer exist. |
| `persist.sys.oplus.brightness.display.type` | `AMOLED` | `AMOLED`: full AOD logic. `IPS`: always OFF when the screen isn't ON. |
| `persist.sys.oplus.brightness.devmax` | *(unset)* | Manual override of the HW max, highest priority |
| `persist.sys.oplus.brightness.devmin` | *(unset)* | Manual override of the HW min, highest priority |
| `persist.sys.oplus.brightness.hw_max` | *(auto)* | Cache of the detected HW max, written by the service |
| `persist.sys.oplus.brightness.hw_min` | *(auto)* | Cache of the detected HW min, written by the service |
| `persist.sys.oplus.brightness.range.max` | `8191` | Cache of the logical (input) brightness max |
| `persist.sys.oplus.brightness.range.min` | `222` | Cache of the logical (input) brightness min |
| `persist.sys.oplus.brightness.lux_aod` | `true` | Enables Lux-AOD auto-brightness logic |
| `persist.sys.oplus.brightness.lux_aod.value` | `8` | HW-units brightness used during Lux AOD |

### Not renamed (already owned by the ROM, left as-is)

* `sys.oplus.multibrightness`, `sys.oplus.multibrightness.min` — live
  IR-detected range, written by the ROM itself.
* `debug.tracing.screen_brightness`, `debug.tracing.screen_state` —
  framework-owned.

## Display types

**IPS LCD:** should work as-is.

**AMOLED:** check `my_product` and remove any props containing `vrr`,
`brightness`, `silky`, `underscreen`, then add in vendor properties:

```
persist.sys.tran.brightness.gammalinear.convert=1
ro.vendor.transsion.backlight_hal.optimization=1
ro.transsion.backlight.level=-1
ro.transsion.physical.backlight.optimization=1
```

## Source layout

```
include/oplus_brightness/   public headers, one per module
src/                        matching .c implementation files
Android.bp                  Soong build rule (cc_binary, vendor: true)
init.oplus.brightness.adaptor.rc
.github/workflows/build.yml NDK cross-compile CI
```
