#ifndef AMIWEATHER_WEATHER_H
#define AMIWEATHER_WEATHER_H

#include <stddef.h>
#include <amiweather/preferences.h>

#define AMIWEATHER_OBSERVATION_TIME_SIZE 32
#define AMIWEATHER_FORECAST_DAYS 5
#define AMIWEATHER_MAX_DAILY 14
#define AMIWEATHER_DATE_SIZE 11
#define AMIWEATHER_COMPACT_DATE_SIZE 7
#define AMIWEATHER_HOURLY_COUNT 12
#define AMIWEATHER_HOURLY_TIME_SIZE 17
#define AMIWEATHER_COMPACT_TIME_SIZE 6
#define AMIWEATHER_VERSION "0.1.0"

enum AmiWeatherIcon {
    AMIWEATHER_ICON_UNKNOWN = 0,
    AMIWEATHER_ICON_CLEAR,
    AMIWEATHER_ICON_PARTLY_CLOUDY,
    AMIWEATHER_ICON_CLOUDY,
    AMIWEATHER_ICON_FOG,
    AMIWEATHER_ICON_DRIZZLE,
    AMIWEATHER_ICON_RAIN,
    AMIWEATHER_ICON_SHOWERS,
    AMIWEATHER_ICON_SNOW,
    AMIWEATHER_ICON_SNOW_SHOWERS,
    AMIWEATHER_ICON_THUNDERSTORM
};

struct AmiWeatherCurrent {
    double temperature_c;
    double apparent_temperature_c;
    double relative_humidity_percent;
    double wind_speed_kmh;
    long weather_code;
    char observation_time[AMIWEATHER_OBSERVATION_TIME_SIZE];
    double wind_direction_degrees;
    double wind_gusts;
    double surface_pressure;
    double cloud_cover_percent;
    double visibility_m;
    double precipitation_mm;
    int has_wind_direction;
    int has_wind_gusts;
    int has_surface_pressure;
    int has_cloud_cover;
    int has_visibility;
    int has_precipitation;
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
    struct AmiWeatherDaily days[AMIWEATHER_MAX_DAILY];
    size_t count;
};

struct AmiWeatherHourly {
    char time[AMIWEATHER_HOURLY_TIME_SIZE];
    double temperature;
    long weather_code;
    int precipitation_probability;
    int has_precipitation_probability;
};

struct AmiWeatherHourlyForecast {
    struct AmiWeatherHourly entries[AMIWEATHER_HOURLY_COUNT];
    size_t count;
};

struct AmiWeatherReport {
    struct AmiWeatherCurrent current;
    struct AmiWeatherForecast forecast;
    struct AmiWeatherHourlyForecast hourly;
    enum AmiWeatherTemperatureUnit temperature_unit;
    enum AmiWeatherWindUnit wind_unit;
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
enum AmiWeatherIcon amiweather_wmo_icon(long weather_code);
const char *amiweather_wmo_compact_description(long weather_code);
int amiweather_format_forecast_date(const char *iso_date,
                                    char *buffer, size_t size);
int amiweather_format_hourly_time(const char *iso_time,
                                  char *buffer, size_t size);
const char *amiweather_wind_direction(double degrees);

#endif
