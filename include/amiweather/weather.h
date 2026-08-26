#ifndef AMIWEATHER_WEATHER_H
#define AMIWEATHER_WEATHER_H

#include <stddef.h>

#define AMIWEATHER_OBSERVATION_TIME_SIZE 32

struct AmiWeatherCurrent {
    double temperature_c;
    double apparent_temperature_c;
    double relative_humidity_percent;
    double wind_speed_kmh;
    long weather_code;
    char observation_time[AMIWEATHER_OBSERVATION_TIME_SIZE];
};

enum AmiWeatherError {
    AMIWEATHER_OK = 0,
    AMIWEATHER_ERR_ARGUMENT,
    AMIWEATHER_ERR_MEMORY,
    AMIWEATHER_ERR_NETWORK,
    AMIWEATHER_ERR_HTTP_STATUS,
    AMIWEATHER_ERR_JSON,
    AMIWEATHER_ERR_MISSING_FIELD,
    AMIWEATHER_ERR_INVALID_FIELD
};

struct AmiWeatherResult {
    enum AmiWeatherError error;
    int http_status;
    char diagnostic[128];
};

const char *amiweather_error_string(enum AmiWeatherError error);

#endif
