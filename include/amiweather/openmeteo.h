#ifndef AMIWEATHER_OPENMETEO_H
#define AMIWEATHER_OPENMETEO_H

#include <stddef.h>
#include <amiweather/location.h>
#include <amiweather/weather.h>

#define AMIWEATHER_REFERENCE_LOCATION "Oslo, Norway"
#define AMIWEATHER_OPENMETEO_BASE_URL "https://api.open-meteo.com/v1/forecast"
#define AMIWEATHER_OPENMETEO_GEOCODING_URL "https://geocoding-api.open-meteo.com/v1/search"

struct AmiWeatherResult amiweather_openmeteo_parse_current(
    const void *json,
    size_t json_size,
    struct AmiWeatherCurrent *current
);

struct AmiWeatherResult amiweather_openmeteo_parse_report(
    const void *json,
    size_t json_size,
    struct AmiWeatherReport *report
);

struct AmiWeatherResult amiweather_openmeteo_fetch_reference(
    struct AmiWeatherCurrent *current
);

struct AmiWeatherResult amiweather_openmeteo_parse_locations(
    const void *json,
    size_t json_size,
    struct AmiWeatherLocationList *locations
);

struct AmiWeatherResult amiweather_geocode_search(
    const char *query,
    struct AmiWeatherLocationList *locations
);

struct AmiWeatherResult amiweather_openmeteo_fetch_current(
    const struct AmiWeatherLocation *location,
    struct AmiWeatherCurrent *current
);

struct AmiWeatherResult amiweather_openmeteo_fetch_report(
    const struct AmiWeatherLocation *location,
    struct AmiWeatherReport *report
);

#endif
