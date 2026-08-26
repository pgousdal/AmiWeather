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
