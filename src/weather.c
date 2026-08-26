#include <amiweather/weather.h>
#include <amiweather/location.h>

#include <stdio.h>
#include <string.h>

const char *amiweather_error_string(enum AmiWeatherError error)
{
    switch (error) {
    case AMIWEATHER_OK: return "success";
    case AMIWEATHER_ERR_ARGUMENT: return "invalid argument";
    case AMIWEATHER_ERR_MEMORY: return "out of memory";
    case AMIWEATHER_ERR_NETWORK: return "network request failed";
    case AMIWEATHER_ERR_HTTP_STATUS: return "unexpected HTTP status";
    case AMIWEATHER_ERR_JSON: return "malformed JSON";
    case AMIWEATHER_ERR_MISSING_FIELD: return "missing weather field";
    case AMIWEATHER_ERR_INVALID_FIELD: return "invalid weather field";
    case AMIWEATHER_ERR_NOT_FOUND: return "no matching locations";
    default: return "unknown error";
    }
}

void amiweather_location_default(struct AmiWeatherLocation *location)
{
    if (!location)
        return;
    memset(location, 0, sizeof(*location));
    snprintf(location->name, sizeof(location->name), "%s", "Oslo");
    snprintf(location->country, sizeof(location->country), "%s", "Norway");
    snprintf(location->country_code, sizeof(location->country_code), "%s", "NO");
    snprintf(location->timezone, sizeof(location->timezone), "%s", "Europe/Oslo");
    location->latitude = 59.9139;
    location->longitude = 10.7522;
}

void amiweather_location_format(const struct AmiWeatherLocation *location,
                                char *buffer, size_t size)
{
    size_t used;
    const char *parts[3];
    size_t i;

    if (!buffer || size == 0)
        return;
    buffer[0] = '\0';
    if (!location)
        return;
    parts[0] = location->name;
    parts[1] = location->admin1;
    parts[2] = location->country;
    for (i = 0; i < 3; ++i) {
        if (!parts[i][0])
            continue;
        used = strlen(buffer);
        if (used && used + 2 < size) {
            buffer[used++] = ',';
            buffer[used++] = ' ';
            buffer[used] = '\0';
        }
        if (used < size - 1)
            strncat(buffer, parts[i], size - used - 1);
    }
}

const char *amiweather_wmo_description(long weather_code)
{
    switch (weather_code) {
    case 0: return "Clear sky";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: return "Fog";
    case 48: return "Rime fog";
    case 51: return "Light drizzle";
    case 53: return "Drizzle";
    case 55: return "Heavy drizzle";
    case 56: return "Light freezing drizzle";
    case 57: return "Freezing drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 66: return "Light freezing rain";
    case 67: return "Freezing rain";
    case 71: return "Light snow";
    case 73: return "Snow";
    case 75: return "Heavy snow";
    case 77: return "Snow grains";
    case 80: return "Light rain showers";
    case 81: return "Rain showers";
    case 82: return "Heavy rain showers";
    case 85: return "Light snow showers";
    case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: return "Thunderstorm, light hail";
    case 99: return "Thunderstorm with hail";
    default: return "Unknown";
    }
}

const char *amiweather_wmo_compact_description(long weather_code)
{
    switch (weather_code) {
    case 56: return "Light frz. drizzle";
    case 57: return "Freezing drizzle";
    case 66: return "Light frz. rain";
    case 67: return "Freezing rain";
    case 80: return "Light showers";
    case 82: return "Heavy showers";
    case 85: return "Light snow shwrs";
    case 86: return "Snow showers";
    case 96: return "T-storm / hail";
    case 99: return "T-storm / hail";
    default: return amiweather_wmo_description(weather_code);
    }
}

enum AmiWeatherIcon amiweather_wmo_icon(long weather_code)
{
    switch (weather_code) {
    case 0: return AMIWEATHER_ICON_CLEAR;
    case 1: case 2: return AMIWEATHER_ICON_PARTLY_CLOUDY;
    case 3: return AMIWEATHER_ICON_CLOUDY;
    case 45: case 48: return AMIWEATHER_ICON_FOG;
    case 51: case 53: case 55: case 56: case 57: return AMIWEATHER_ICON_DRIZZLE;
    case 61: case 63: case 65: case 66: case 67: return AMIWEATHER_ICON_RAIN;
    case 80: case 81: case 82: return AMIWEATHER_ICON_SHOWERS;
    case 71: case 73: case 75: case 77: return AMIWEATHER_ICON_SNOW;
    case 85: case 86: return AMIWEATHER_ICON_SNOW_SHOWERS;
    case 95: case 96: case 99: return AMIWEATHER_ICON_THUNDERSTORM;
    default: return AMIWEATHER_ICON_UNKNOWN;
    }
}

static int leap_year(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int amiweather_format_forecast_date(const char *iso_date,
                                    char *buffer, size_t size)
{
    static const char *weekdays[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const int month_days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int year, month, day, max_day;
    int adjusted_month, adjusted_year, weekday;

    if (!iso_date || !buffer || size < AMIWEATHER_COMPACT_DATE_SIZE)
        return 0;
    if (strlen(iso_date) != 10 || iso_date[4] != '-' || iso_date[7] != '-' ||
        sscanf(iso_date, "%4d-%2d-%2d", &year, &month, &day) != 3 ||
        year < 1 || month < 1 || month > 12)
        return 0;
    max_day = month_days[month - 1];
    if (month == 2 && leap_year(year))
        ++max_day;
    if (day < 1 || day > max_day)
        return 0;
    adjusted_month = month;
    adjusted_year = year;
    if (adjusted_month < 3) {
        adjusted_month += 12;
        --adjusted_year;
    }
    weekday = (day + (13 * (adjusted_month + 1)) / 5 + adjusted_year +
               adjusted_year / 4 - adjusted_year / 100 + adjusted_year / 400) % 7;
    weekday = (weekday + 6) % 7;
    snprintf(buffer, size, "%s %02d", weekdays[weekday], day);
    return 1;
}

int amiweather_format_hourly_time(const char *iso_time,
                                  char *buffer, size_t size)
{
    char date[AMIWEATHER_DATE_SIZE];
    char compact_date[AMIWEATHER_COMPACT_DATE_SIZE];
    int hour, minute;
    if (!iso_time || !buffer || size < AMIWEATHER_COMPACT_TIME_SIZE ||
        strlen(iso_time) != 16 || iso_time[10] != 'T' || iso_time[13] != ':')
        return 0;
    memcpy(date, iso_time, 10);
    date[10] = '\0';
    if (!amiweather_format_forecast_date(date, compact_date,
                                         sizeof(compact_date)) ||
        sscanf(iso_time + 11, "%2d:%2d", &hour, &minute) != 2 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59)
        return 0;
    snprintf(buffer, size, "%02d:%02d", hour, minute);
    return 1;
}
