# AmiWeather

AmiWeather is a native weather application for classic AmigaOS. The primary
target is AmigaOS 3.x on a 68020 or newer processor; no FPU or MMU is required.
M0 is version 0.0.1 and establishes the project architecture, a real
Open-Meteo client, deterministic parsing tests, a small POSIX reference
program, and a native GUI shell. It is not a user-ready release.

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

## M0 behaviour and limitations

The POSIX reference executable requests current conditions for the documented
demo location, Oslo, Norway, and prints temperature, apparent temperature,
humidity, wind speed, WMO weather code, and observation time. It is a
development aid, not a second product. The native M0 executable is deliberately
a small Workbench/public-screen GUI shell; live networking is not yet performed
inside its synchronous Intuition event loop.

M0 has no location search, forecast presentation, icons, persistence,
preferences, localization, background refresh, or asynchronous networking.
