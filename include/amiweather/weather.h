#ifndef AMIWEATHER_WEATHER_H
#define AMIWEATHER_WEATHER_H

#include <stddef.h>

#define AMIWEATHER_OBSERVATION_TIME_SIZE 32
#define AMIWEATHER_FORECAST_DAYS 5
#define AMIWEATHER_DATE_SIZE 11
#define AMIWEATHER_COMPACT_DATE_SIZE 7

struct AmiWeatherCurrent {
    double temperature_c;
    double apparent_temperature_c;
    double relative_humidity_percent;
    double wind_speed_kmh;
    long weather_code;
    char observation_time[AMIWEATHER_OBSERVATION_TIME_SIZE];
};

struct AmiWeatherDaily {
    char date[AMIWEATHER_DATE_SIZE];
    double temperature_min_c;
    double temperature_max_c;
    long weather_code;
    int precipitation_probability;
    int has_precipitation_probability;
};

struct AmiWeatherForecast {
    struct AmiWeatherDaily days[AMIWEATHER_FORECAST_DAYS];
    size_t count;
};

struct AmiWeatherReport {
    struct AmiWeatherCurrent current;
    struct AmiWeatherForecast forecast;
};

enum AmiWeatherError {
    AMIWEATHER_OK = 0,
    AMIWEATHER_ERR_ARGUMENT,
    AMIWEATHER_ERR_MEMORY,
    AMIWEATHER_ERR_NETWORK,
    AMIWEATHER_ERR_HTTP_STATUS,
    AMIWEATHER_ERR_JSON,
    AMIWEATHER_ERR_MISSING_FIELD,
    AMIWEATHER_ERR_INVALID_FIELD,
    AMIWEATHER_ERR_NOT_FOUND
};

struct AmiWeatherResult {
    enum AmiWeatherError error;
    int http_status;
    char diagnostic[128];
};

const char *amiweather_error_string(enum AmiWeatherError error);
const char *amiweather_wmo_description(long weather_code);
const char *amiweather_wmo_compact_description(long weather_code);
int amiweather_format_forecast_date(const char *iso_date,
                                    char *buffer, size_t size);

#endif
