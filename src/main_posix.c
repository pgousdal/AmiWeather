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
    struct AmiWeatherPreferences preferences;
    const char *place_query = NULL;
    int argument = 1;

    amiweather_preferences_default(&preferences);
    if (argc > 2 && strcmp(argv[1], "--preferences") == 0) {
        enum AmiWeatherPreferencesError preferences_result =
            amiweather_preferences_load_file(argv[2], &preferences);
        if (preferences_result != AMIWEATHER_PREFS_OK &&
            preferences_result != AMIWEATHER_PREFS_NOT_FOUND) {
            fprintf(stderr, "AmiWeather preferences: %s\n",
                    amiweather_preferences_error_string(preferences_result));
            return 1;
        }
        argument = 3;
    }
    if (argument < argc)
        place_query = argv[argument];

    if (place_query) {
        result = amiweather_geocode_search(place_query, &locations);
        if (result.error != AMIWEATHER_OK) {
            fprintf(stderr, "AmiWeather search: %s: %s\n",
                    amiweather_error_string(result.error), result.diagnostic);
            return 1;
        }
        location = locations.locations[0];
    } else if (preferences.has_default_location) {
        location = preferences.saved_locations[preferences.default_location_index];
    } else {
        amiweather_location_default(&location);
    }
    result = amiweather_openmeteo_fetch_report_units(
        &location, preferences.temperature_unit, preferences.wind_unit, &report);
    if (result.error != AMIWEATHER_OK) {
        fprintf(stderr, "AmiWeather: %s: %s\n",
                amiweather_error_string(result.error), result.diagnostic);
        return 1;
    }
    amiweather_location_format(&location, display, sizeof(display));
    printf("AmiWeather 0.0.1 - %s\n", display);
    printf("Temperature: %.1f %s\n", report.current.temperature_c,
           amiweather_temperature_unit_label(preferences.temperature_unit));
    printf("Feels like: %.1f %s\n", report.current.apparent_temperature_c,
           amiweather_temperature_unit_label(preferences.temperature_unit));
    printf("Humidity: %.0f %%\n", report.current.relative_humidity_percent);
    printf("Wind: %.1f %s\n", report.current.wind_speed_kmh,
           amiweather_wind_unit_label(preferences.wind_unit));
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
            printf("%s  %-24s  %.1f / %.1f %s",
                   day->date, amiweather_wmo_description(day->weather_code),
                   day->temperature_min_c, day->temperature_max_c,
                   amiweather_temperature_unit_label(preferences.temperature_unit));
            if (day->has_precipitation_probability)
                printf("  %d%%", day->precipitation_probability);
            putchar('\n');
        }
    }
    puts("\nHourly:");
    {
        size_t i;
        for (i = 0; i < report.hourly.count; ++i) {
            const struct AmiWeatherHourly *entry = &report.hourly.entries[i];
            char time[AMIWEATHER_COMPACT_TIME_SIZE];
            amiweather_format_hourly_time(entry->time, time, sizeof(time));
            printf("%s  %-20s  %.1f %s", time,
                   amiweather_wmo_description(entry->weather_code),
                   entry->temperature,
                   amiweather_temperature_unit_label(preferences.temperature_unit));
            if (entry->has_precipitation_probability)
                printf("  %d%%", entry->precipitation_probability);
            putchar('\n');
        }
    }
    return 0;
}
