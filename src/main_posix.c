#include <amiweather/openmeteo.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct AmiWeatherLocation location;
    struct AmiWeatherLocationList locations;
    struct AmiWeatherReport report;
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
    result = amiweather_openmeteo_fetch_report(&location, &report);
    if (result.error != AMIWEATHER_OK) {
        fprintf(stderr, "AmiWeather: %s: %s\n",
                amiweather_error_string(result.error), result.diagnostic);
        return 1;
    }
    amiweather_location_format(&location, display, sizeof(display));
    printf("AmiWeather 0.0.1 - %s\n", display);
    printf("Temperature: %.1f C\n", report.current.temperature_c);
    printf("Feels like: %.1f C\n", report.current.apparent_temperature_c);
    printf("Humidity: %.0f %%\n", report.current.relative_humidity_percent);
    printf("Wind: %.1f km/h\n", report.current.wind_speed_kmh);
    printf("Condition: %s (WMO %ld)\n",
           amiweather_wmo_description(report.current.weather_code),
           report.current.weather_code);
    printf("Observation: %s (%s)\n", report.current.observation_time,
           location.timezone[0] ? location.timezone : "local time");
    puts("\nForecast:");
    {
        size_t i;
        for (i = 0; i < report.forecast.count; ++i) {
            const struct AmiWeatherDaily *day = &report.forecast.days[i];
            printf("%s  %-24s  %.1f / %.1f C",
                   day->date, amiweather_wmo_description(day->weather_code),
                   day->temperature_min_c, day->temperature_max_c);
            if (day->has_precipitation_probability)
                printf("  %d%%", day->precipitation_probability);
            putchar('\n');
        }
    }
    return 0;
}
