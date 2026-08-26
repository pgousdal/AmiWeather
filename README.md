# AmiWeather

AmiWeather is a native weather application for classic AmigaOS. The primary
target is AmigaOS 3.x on a 68020 or newer processor; no FPU or MMU is required.
The current M5 build remains version 0.0.1. It provides persistent preferences,
saved locations, current conditions, five daily rows, and a twelve-entry hourly
forecast, while remaining an early build
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

## Locations, preferences, and forecast

The native application opens on the user's public screen and fetches current
weather and forecast for the configured default, or Oslo on first run.
**Change...** opens a compact native GadTools
search window. It searches Open-Meteo's geocoding API, presents up to eight
`place, region, country` matches, and fetches weather before committing a
selection. A failed first fetch leaves the previous location, weather, and
forecast intact. **Locations** manages up to eight saved locations. The current
location can be added without duplicating an existing match, and saved entries
can be selected without another geocoding request, removed, or marked as the
startup default. Removing the default selects the first remaining saved entry
as default; removing the last saved entry restores the built-in Oslo startup
fallback. It displays temperature, apparent
temperature,
relative humidity, wind speed, a human-readable condition derived from the WMO
weather code, and observation time local to the selected location when its
timezone is available. A compact native table adds up to five daily rows with
weekday/date, WMO condition, minimum and maximum temperatures, and
precipitation probability when supplied and space permits. Current and daily
data use one Open-Meteo request and are committed together. The GadTools
**Refresh** button (or the `R` key) updates all three forecast sections. Loading and concise errors are
shown without discarding the last successful display; Intuition exposure
redraws only stored state and does not initiate network traffic.

**Hourly...** opens a compact native GadTools list showing up to twelve local
hours with time, compact WMO condition, temperature, and precipitation
probability when available. The adapter selects the current forecast hour—the
first validated hourly timestamp whose `YYYY-MM-DDTHH` is at or after the
current observation hour—and then at most eleven following entries. This
naturally crosses midnight, month, and year boundaries without POSIX date APIs.
The listview scrolls when the screen font or height cannot show all rows.

The Preferences window selects Celsius or Fahrenheit and km/h or mph. Unit
changes fetch a complete current-plus-daily report and persist the candidate
preferences before changing displayed state. A failed fetch or save leaves the
old units and report intact. Amiga decimal rendering remains integer-based.

Requests are synchronous in M5. Current, daily, and hourly fields use one
Open-Meteo response and commit together. Search, a selection's initial weather fetch,
launch, and Refresh can temporarily block GUI interaction until the request
completes or AmiAPI's finite 20-second network
timeout expires. Background/asynchronous requests may be evaluated later.

The POSIX reference executable accepts `--preferences PATH` followed by an
optional place query. It never reads the developer's home directory. It selects the
first result for smoke testing, and prints its current weather and forecast. It remains a
development and live-smoke-test aid, not a second product.

Geocoding strings are kept as validated UTF-8 in bounded domain buffers.
Classic screen fonts are not Unicode-aware, so unavailable non-Latin glyphs
may display incorrectly and clipping is byte-oriented. Coordinates and API
queries are unaffected; M4 does not add a general Unicode conversion layer.

## Preference file

Classic Amiga builds read `ENV:AmiWeather/preferences` first and otherwise
`ENVARC:AmiWeather/preferences`. Saving publishes the persistent ENVARC copy
first and then mirrors ENV; a failed ENV mirror is removed so the next startup
falls back to ENVARC. Missing files are normal first-run state. Malformed or
unsupported files are not overwritten at launch; AmiWeather falls back safely.

Format version 1 is bounded, line-oriented text with explicit `version`,
`temperature`, `wind`, optional `default`, and repeated `location` records.
Location fields retain name, region, country, country code, decimal coordinates,
and timezone. Unknown keys are ignored for forward compatibility. Saving writes
and closes a `.new` file, moves an existing file to `.old`, publishes the new
file, rolls back on publication failure, and removes the backup after success.
This is the safest portable strategy used because rename-over-existing behavior
is not guaranteed across classic Amiga filesystems.

Forecast storage is fixed at five daily and twelve hourly application-owned
entries. M5 has no charts, maps, custom icons, localization, asynchronous
networking, or background refresh.
