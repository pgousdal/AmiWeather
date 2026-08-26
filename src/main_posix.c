#include <amiweather/openmeteo.h>
#include <stdio.h>

int main(void)
{
    struct AmiWeatherCurrent weather;
    struct AmiWeatherResult result = amiweather_openmeteo_fetch_reference(&weather);
    if (result.error != AMIWEATHER_OK) {
        fprintf(stderr, "AmiWeather: %s: %s\n",
                amiweather_error_string(result.error), result.diagnostic);
        return 1;
    }
    printf("AmiWeather 0.0.1 - %s\n", AMIWEATHER_REFERENCE_LOCATION);
    printf("Temperature: %.1f C\n", weather.temperature_c);
    printf("Feels like: %.1f C\n", weather.apparent_temperature_c);
    printf("Humidity: %.0f %%\n", weather.relative_humidity_percent);
    printf("Wind: %.1f km/h\n", weather.wind_speed_kmh);
    printf("Condition: %s (WMO %ld)\n",
           amiweather_wmo_description(weather.weather_code),
           weather.weather_code);
    printf("Observation: %s UTC\n", weather.observation_time);
    return 0;
}
