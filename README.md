# AmiWeather

AmiWeather is a native weather application for classic AmigaOS. The primary
target is AmigaOS 3.x on a 68020 or newer processor; no FPU or MMU is required.
The current M7.1 release candidate is version 0.1.0. It provides persistent preferences,
saved locations, current conditions, five daily rows, and a twelve-entry hourly
forecast, while remaining an early build
and a native 12-entry hourly forecast.

M8.0/M8.2 add a separate `AmiWeatherDoor` terminal program. It shares the
weather/domain and Open-Meteo core with the GUI and provides a plain-text,
host-testable presentation layer. The documented ABBS 3.3 Shell-door launcher
uses no command-line context and fails closed through `EndCLI`; the interactive
door is runtime-qualified on ABBS 3.3.

The Amiga interface uses Intuition and GadTools on the user's public screen.
It has **no MUI, ReAction, SDL, Qt, GTK, or portable GUI-toolkit dependency**.
AmiWeather consumes [AmiAPI](../AmiAPI/README.md) as an external SDK and does
not include its sources or private headers. HTTPS on AmigaOS therefore needs a
bsdsocket.library-compatible TCP/IP stack and AmiSSL 5, as required by AmiAPI.
AmiWeather supports AmigaOS 3.1 and internally provisions a 32 KiB runtime
stack before entering the GUI/network call tree; users do not need to run a
Shell `Stack` command. The embedded `$STACK:32768` token is supplementary and
is not required for OS 3.1 correctness.

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

The Amiga build produces both `build/amiga/AmiWeather` and
`build/amiga/AmiWeatherDoor`. The door uses no Intuition or GadTools.

## ABBS 3.3 door integration

`contrib/abbs/` contains an ABBS 3.3 Shell-door script and example
Node0Config/Node0Menu entries. Copy `AmiWeatherDoor` and the script to a fixed
ABBS drawer, add the `S` configuration entry, and use `OPEN n` from ABBS. The
launcher passes no user-controlled arguments and always runs `EndCLI`.
The door provides the runtime-qualified interactive H/D/7/A/W/L/R/Q weather
session on ABBS 3.3. No AmiExpress support is claimed.

`AMIAPI_DIR` defaults to `../AmiAPI`. The build checks for its public headers
and `build/libamiapi.a` and gives a direct error if either is absent. It never
copies or rebuilds AmiAPI.

## Locations, preferences, and forecast

The native application opens on the user's public screen and fetches current
weather and forecast for the configured default. With no configured default it
shows `No default location configured.` and leaves **Change...** available;
it does not fetch Oslo or geolocate the user.
**Change...** opens a compact native GadTools
search window. It searches Open-Meteo's geocoding API, presents up to eight
`place, region, country` matches, and fetches weather before committing a
selection. A failed first fetch leaves the previous location, weather, and
forecast intact. **Locations** manages up to eight saved locations. The current
location can be added without duplicating an existing match, and saved entries
can be selected without another geocoding request, removed, or marked as the
startup default. **Set Current as Default** adds the active location when
needed, makes it the default, and persists the complete preference state.
Removing the final saved entry leaves no configured default. It displays temperature, apparent
temperature,
relative humidity, wind speed, a human-readable condition derived from the WMO
weather code, and observation time local to the selected location when its
timezone is available. A compact native table adds up to five daily rows with
weekday/date, WMO condition, minimum and maximum temperatures, and
precipitation probability when supplied and space permits. Current, daily, and
hourly data use one Open-Meteo request and are committed together. The GadTools
**Refresh** button (or the `R` key) updates all three forecast sections. Loading and concise errors are
shown without discarding the last successful display; Intuition exposure
redraws only stored state and does not initiate network traffic.

**Hourly...** opens a compact native GadTools list showing up to twelve local
hours with time, compact WMO condition, temperature, and precipitation
probability when available. The adapter selects the current forecast hour—the
first validated hourly timestamp whose `YYYY-MM-DDTHH` is at or after the
current observation hour—and then at most eleven following entries. This
naturally crosses midnight, month, and year boundaries without POSIX date APIs.
The listview scrolls when the screen font or height cannot show all rows. The
current block and daily rows use a cohesive compiled-in few-color line-art icon
set mapped centrally from WMO codes; hourly rows remain text-first for clarity.

The Preferences window selects Celsius or Fahrenheit and km/h or mph. Unit
changes fetch a complete current-plus-daily report and persist the candidate
preferences before changing displayed state. A failed fetch or save leaves the
old units and report intact. Amiga decimal rendering remains integer-based.

Requests are synchronous in M6. Current, daily, and hourly fields use one
Open-Meteo response and commit together. Search, a selection's initial weather fetch,
launch, and Refresh can temporarily block GUI interaction until the request
completes or AmiAPI's finite 20-second network
timeout expires. Background/asynchronous requests may be evaluated later.

The main window keeps Refresh, Change..., and Hourly... as visible frequent
actions. Locations and Preferences are available from the native menu bar.
Project also contains About... and Quit; Location contains Change... and Saved
Locations..., Forecast contains Hourly..., and Settings contains Preferences....

The POSIX reference executable accepts `--preferences PATH` followed by an
optional place query. It never reads the developer's home directory. It selects the
first result for smoke testing, and prints its current weather and forecast. It remains a
development and live-smoke-test aid, not a second product.

Geocoding strings are kept as validated UTF-8 in bounded domain buffers.
Classic screen fonts are not Unicode-aware, so unavailable non-Latin glyphs
may display incorrectly and clipping is byte-oriented. Coordinates and API
queries are unaffected; M6 does not add a general Unicode conversion layer.

## Preference file

Classic Amiga builds read `ENV:AmiWeather/preferences` first and otherwise
`ENVARC:AmiWeather/preferences`. GUI and Door call the same shared resolver.
Explicit Save publishes the persistent ENVARC copy first and then mirrors ENV,
and reports failure if either publication fails. If the ENV publication fails,
the previous ENVARC state is restored where possible. Missing files are normal
first-run state. Malformed or
unsupported files are not overwritten at launch; AmiWeather falls back safely.

Format version 1 is bounded, line-oriented text with explicit `version`,
`temperature`, `wind`, optional `default`, and repeated `location` records.
Location fields retain name, region, country, country code, decimal coordinates,
and timezone. Unknown keys are ignored for forward compatibility. Saving writes
and closes a `.new` file, moves an existing file to `.old`, publishes the new
file, rolls back on publication failure, and removes the backup after success.
This is the safest portable strategy used because rename-over-existing behavior
is not guaranteed across classic Amiga filesystems.

## ARexx

While the GUI is running it owns the public `AMIWEATHER` port. Supported V1
commands are `VERSION`, `STATUS`, `REFRESH`, `GET LOCATION`, `GET TEMPERATURE`,
`GET CONDITION`, `GET HUMIDITY`, `GET WIND`, `LOCATION <search text>`,
`SETDEFAULT`, `SET TEMPUNIT C|F`, `SET WINDUNIT KMH|MPH`, and `SAVE`.
Queries return bounded text in `RESULT`; successful mutations return `OK`.
Return code 10 means unknown command, 11 invalid arguments, 12 no report, 13 no
location, 14 ambiguous location, 15 geocoding failure, 16 save failure, and 17
an unavailable operation/state. `LOCATION` commits only when geocoding returns
exactly one result and its weather fetch succeeds; it never guesses among
multiple results and does not persist without `SETDEFAULT` or `SAVE`.

Example scripts are in `contrib/arexx/`. The GUI owns automation because it is
the persistent application. The short-lived AmiWeatherDoor consumes the same
preferences but intentionally has no ARexx port.

Forecast storage is fixed at fourteen daily and twelve hourly application-owned
entries; the main view remains five daily rows. M7 has no charts, maps,
animations, localization, asynchronous
networking, or background refresh. A Workbench `.info` icon is deliberately
deferred because no reliable icon-generation toolchain is available in the
build environment.

## Aminet release

After building the Amiga AmiAPI archive, run `make release PLATFORM=amiga`.
This creates the two Aminet upload artifacts `dist/AmiWeather-0.1.0.lha` and
`dist/AmiWeather-0.1.0.readme`, plus its local `.sha256` sidecar and a clean
staging tree. The release script generates one canonical Aminet readme with
hashes from the exact staged executable, copies it into the archive, creates an
LHA archive with `jlha`, and the validation script tests/list-extracts it and
verifies the executable hash.
The archive is not uploaded automatically.

Generate the standalone qualified door package with `make door-release` after
the Amiga `AmiWeatherDoor` binary is present. This creates the LHA, external
Aminet `.readme`, and local SHA-256 sidecar without rebuilding the executable.

## Contact

Project contact: amiweather@ousdal.org
