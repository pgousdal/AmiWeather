# AmiWeather

AmiWeather is a native weather application for classic AmigaOS. The primary
target is AmigaOS 3.x on a 68020 or newer processor; no FPU or MMU is required.
The current M1 build remains version 0.0.1. It provides current conditions for
the fixed reference location Oslo, Norway, while remaining an early build
rather than a user-ready release.

The Amiga interface uses Intuition and GadTools on the user's public screen.
It has **no MUI, ReAction, SDL, Qt, GTK, or portable GUI-toolkit dependency**.
AmiWeather consumes [AmiAPI](../AmiAPI/README.md) as an external SDK and does
not include its sources or private headers. HTTPS on AmigaOS therefore needs a
bsdsocket.library-compatible TCP/IP stack and AmiSSL 5, as required by AmiAPI.

## Build

Build the sibling AmiAPI library for the same platform first, then run:

```sh
make PLATFORM=posix AMIAPI_DIR=../AmiAPI
make PLATFORM=posix test AMIAPI_DIR=../AmiAPI
make PLATFORM=posix smoke AMIAPI_DIR=../AmiAPI  # live network request
```

For the native cross-build (using `m68k-amigaos-gcc` and `-m68020`):

```sh
make PLATFORM=amiga AMIAPI_DIR=../AmiAPI
```

`AMIAPI_DIR` defaults to `../AmiAPI`. The build checks for its public headers
and `build/libamiapi.a` and gives a direct error if either is absent. It never
copies or rebuilds AmiAPI.

## Current conditions

The native application opens on the user's public screen and fetches current
weather for Oslo at launch. It displays temperature, apparent temperature,
relative humidity, wind speed, a human-readable condition derived from the WMO
weather code, and the UTC observation time. The GadTools **Refresh** button (or
the `R` key) requests a new observation. Loading and network/error states are
shown in the window; Intuition exposure redraws only the stored state and does
not initiate network traffic.

Requests are synchronous in M1. Launch and Refresh can temporarily block GUI
interaction until the request completes or AmiAPI's finite 20-second network
timeout expires. Background/asynchronous requests may be evaluated later.

The POSIX reference executable exercises the same domain and Open-Meteo code
and prints the condition description together with the WMO code. It remains a
development and live-smoke-test aid, not a second product.

M1 has no geocoding or location selection, forecasts, weather icons,
persistence, preferences, localization, or background refresh.
