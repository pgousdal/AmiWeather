#ifndef AMIWEATHER_OPENMETEO_H
#define AMIWEATHER_OPENMETEO_H

#include <stddef.h>
#include <amiweather/weather.h>

#define AMIWEATHER_REFERENCE_LOCATION "Oslo, Norway"
#define AMIWEATHER_OPENMETEO_BASE_URL "https://api.open-meteo.com/v1/forecast"

struct AmiWeatherResult amiweather_openmeteo_parse_current(
    const void *json,
    size_t json_size,
    struct AmiWeatherCurrent *current
);

struct AmiWeatherResult amiweather_openmeteo_fetch_reference(
    struct AmiWeatherCurrent *current
);

#endif
