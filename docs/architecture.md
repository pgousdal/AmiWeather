# Architecture

```text
Intuition + GadTools UI
          |
    AmiWeather domain
          |
   Open-Meteo adapter
          |
        AmiAPI
          |
 bsdsocket + AmiSSL 5
```

The GUI consumes application-owned `AmiWeatherPreferences` and an
`AmiWeatherReport`, containing current
conditions and a fixed-capacity five-entry daily forecast, and a bounded
`AmiWeatherLocation`, plus application-level result codes.
Open-Meteo field names and AmiAPI JSON
objects stay in the adapter. AmiAPI is an external SDK: AmiWeather includes
only `include/amiapi/*` public headers and links `libamiapi.a`; it neither
compiles AmiAPI sources nor accesses opaque implementation structures.

The adapter builds weather and geocoding HTTPS URLs with AmiAPI's query
builder, bounds the response to 64 KiB, applies a 20-second network timeout,
checks for a 2xx HTTP status, and parses the response with AmiAPI's public JSON
API. All acquired network objects unwind through one cleanup path.

M4 stores an in-memory selected location, initially the configured default or
Oslo. Geocoding requests
up to eight English-labelled results and requires a nonempty name plus valid
coordinates; region, country, country code, and timezone are optional. Weather
uses the selected coordinates and returned timezone, or Open-Meteo's `auto`
policy when absent. A candidate becomes current only after its initial weather
fetch succeeds. One weather query requests the existing current fields plus
daily time, WMO code, minimum/maximum temperature, and optional maximum
precipitation probability, with `forecast_days=5`. Required parallel arrays
must exist, have equal nonzero lengths, and contain valid entries. More than
five server entries are safely bounded. Optional precipitation entries may be
absent or null; present values must be in the range 0 through 100.

The GUI models loading, valid-weather, and error presentation explicitly. A
response is parsed into a temporary report and current plus forecast are
committed atomically only after complete validation. Failed Refresh retains and
continues displaying the last successful report with concise status. A failed
candidate fetch retains the prior location and report. The GUI stores only
AmiWeather domain values and application-level results. Launch,
search, selection, and explicit Refresh actions call the synchronous adapter;
repainting reads the
stored state and never performs network I/O. A request can therefore block GUI
interaction until completion or timeout in M4. Background execution remains a
later design question rather than premature infrastructure.

## Native UI direction

AmiWeather should look native to the user's Amiga environment instead of
imposing a theme. It uses Intuition conventions, GadTools gadgets, system
fonts and pens, standard window borders, and the user-selected Workbench or
public screen. Layouts must remain usable at low resolutions and must not
assume square pixels, a fixed palette, a fixed font, RTG, or true colour.
Standard menus and keyboard behaviour should follow normal Amiga conventions.

Restrained custom drawing is appropriate for future weather symbols or
forecast visualization, but it must not replace standard gadgets merely for
appearance. The goal is a polished classic AmigaOS 3.x application, neither a
developer-only shell nor a modern interface transplanted onto AmigaOS.

The native application locks the default public screen, derives its layout from
the screen bounds, borders, and font metrics, obtains GadTools VisualInfo,
creates standard Refresh and Change buttons plus a modal search window using
GadTools string, listview, and button gadgets. Its fixed result buffers and
list nodes remain valid for the gadget lifetime and are detached before reuse.
It processes IDCMP messages and
releases the window, gadgets,
VisualInfo, screen lock, and libraries in reverse order. Large response data is
heap-owned by AmiAPI; AmiWeather has no large automatic buffers or recursive
application architecture. Forecast rows are drawn through RastPort in JAM1
using system pens and font-derived spacing. The deterministic Gregorian date
helper has no POSIX date dependency. Canonical WMO descriptions are shared with
current conditions; one presentation helper shortens only labels that do not
fit compact rows. Narrow windows omit precipitation before core date,
condition, and temperature data. The 640x256 layout accommodates five rows
above the gadgets with a standard system font; 640x512 retains the same compact
layout.

Amiga temperature presentation converts domain doubles to rounded integer
tenths and uses only integer `snprintf` conversions, preserving the 68020+
no-FPU policy. The bounded report is modest persistent application state;
response JSON stays heap-owned by AmiAPI and no large forecast arrays are put
on the stack. Resources are unchanged and still unwind in reverse order.

M4 adds compact GadTools Locations and Preferences windows while keeping the
weather and forecast primary. Locations supports Use, Add Current, Remove, and
Default over eight fixed entries. Use performs no geocoding and retains the
fetch-before-commit behavior. List mutations operate on a copy and commit in
memory only after persistence succeeds. Duplicate identity is name, country
code, timezone, and coordinates rounded to six decimals. Removing an item
compacts the array and adjusts a later default index; removing the default
chooses the first remaining entry, while an empty list falls back to Oslo.

The Preferences window uses GadTools cycle gadgets for Celsius/Fahrenheit and
km/h/mph. Open-Meteo receives both selected unit parameters in the same query,
so current and daily temperatures share a unit. Changes use fetch-candidate,
publish-preferences, then commit report and preferences. Any failure leaves the
old display and settings coherent. Secondary actions remain buttons: a menu bar
would add menu lifecycle and event scope without enough benefit to justify
destabilizing the existing compact UI in M4.

## Preference persistence

The parser and explicit-path file I/O are platform-neutral project code; Amiga
path policy remains in the native frontend. Version 1 is bounded line-oriented
text, never raw struct data, and records units, optional default index, and
pipe-delimited location fields with decimal coordinates. Lines are limited to
511 bytes, files to 8 KiB, locations to eight, reserved delimiters are rejected,
unknown keys are ignored, and parsing commits only after the whole file
validates. Missing files are normal; malformed or unsupported files preserve
usability and are not overwritten merely by launching.

Startup prefers `ENV:AmiWeather/preferences`, then
`ENVARC:AmiWeather/preferences`. Save creates missing AmiWeather drawers,
publishes ENVARC first, and mirrors ENV. If the mirror fails it is removed,
leaving ENVARC unambiguous on fallback. Each file is written to `.new`, flushed
and closed, the prior target is moved to `.old`, and the new file is renamed
into place. Publication failure attempts rollback; success removes the backup.
This avoids exposing a partially written open file without assuming atomic
rename-over-existing behavior on every classic Amiga filesystem.

Geocoding text remains validated UTF-8. Classic Amiga fonts generally do not
render Unicode and clipping is byte-oriented, so M4 documents imperfect
non-Latin display rather than adding a large conversion framework. Memory
safety, coordinates, and request queries are unaffected.
