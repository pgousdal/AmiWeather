#include <amiweather/openmeteo.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct AmiWeatherLocation location;
    struct AmiWeatherLocationList locations;
    struct AmiWeatherCurrent weather;
    struct AmiWeatherResult result;
    char display[AMIWEATHER_LOCATION_DISPLAY_SIZE];

    if (argc > 1) {
        result = amiweather_geocode_search(argv[1], &locations);
        if (result.error != AMIWEATHER_OK) {
            fprintf(stderr, "AmiWeather search: %s: %s\n",
                    amiweather_error_string(result.error), result.diagnostic);
            return 1;
        }
        location = locations.locations[0];
    } else {
        amiweather_location_default(&location);
    }
    result = amiweather_openmeteo_fetch_current(&location, &weather);
    if (result.error != AMIWEATHER_OK) {
        fprintf(stderr, "AmiWeather: %s: %s\n",
                amiweather_error_string(result.error), result.diagnostic);
        return 1;
    }
    amiweather_location_format(&location, display, sizeof(display));
    printf("AmiWeather 0.0.1 - %s\n", display);
    printf("Temperature: %.1f C\n", weather.temperature_c);
    printf("Feels like: %.1f C\n", weather.apparent_temperature_c);
    printf("Humidity: %.0f %%\n", weather.relative_humidity_percent);
    printf("Wind: %.1f km/h\n", weather.wind_speed_kmh);
    printf("Condition: %s (WMO %ld)\n",
           amiweather_wmo_description(weather.weather_code),
           weather.weather_code);
    printf("Observation: %s (%s)\n", weather.observation_time,
           location.timezone[0] ? location.timezone : "local time");
    return 0;
}
