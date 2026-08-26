#include <amiweather/weather.h>

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
    default: return "unknown error";
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
