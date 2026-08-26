# AmiWeather

AmiWeather is a native weather application for classic AmigaOS. The primary
target is AmigaOS 3.x on a 68020 or newer processor; no FPU or MMU is required.
The current M3 build remains version 0.0.1. It provides location search,
current conditions, and a five-day forecast, while remaining an early build
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

## Location search, current conditions, and forecast

The native application opens on the user's public screen and fetches current
weather and forecast for Oslo at launch. **Change...** opens a compact native GadTools
search window. It searches Open-Meteo's geocoding API, presents up to eight
`place, region, country` matches, and fetches weather before committing a
selection. A failed first fetch leaves the previous location, weather, and
forecast intact. The selected location is retained in memory so Refresh uses it for the
rest of the run; it is not yet persisted. It displays temperature, apparent
temperature,
relative humidity, wind speed, a human-readable condition derived from the WMO
weather code, and observation time local to the selected location when its
timezone is available. A compact native table adds up to five daily rows with
weekday/date, WMO condition, minimum and maximum temperatures, and
precipitation probability when supplied and space permits. Current and daily
data use one Open-Meteo request and are committed together. The GadTools
**Refresh** button (or the `R` key) updates both. Loading and concise errors are
shown without discarding the last successful display; Intuition exposure
redraws only stored state and does not initiate network traffic.

Requests are synchronous in M3. Search, a selection's initial weather fetch,
launch, and Refresh can temporarily block GUI interaction until the request
completes or AmiAPI's finite 20-second network
timeout expires. Background/asynchronous requests may be evaluated later.

The POSIX reference executable accepts an optional place query, selects the
first result for smoke testing, and prints its current weather and forecast. It remains a
development and live-smoke-test aid, not a second product.

Geocoding strings are kept as validated UTF-8 in bounded domain buffers.
Classic screen fonts are not Unicode-aware, so unavailable non-Latin glyphs
may display incorrectly and clipping is byte-oriented. Coordinates and API
queries are unaffected; M3 does not add a general Unicode conversion layer.

Forecast storage is fixed at five application-owned entries. M3 has no hourly
forecast, graphs, maps, custom icons, persistence, favourites, preferences,
localization, or background refresh.
