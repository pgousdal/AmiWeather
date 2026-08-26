#ifndef AMIWEATHER_PREFERENCES_H
#define AMIWEATHER_PREFERENCES_H

#include <stddef.h>
#include <amiweather/location.h>

#define AMIWEATHER_PREFERENCES_VERSION 1U
#define AMIWEATHER_MAX_SAVED_LOCATIONS 8
#define AMIWEATHER_PREFERENCES_LINE_SIZE 512

enum AmiWeatherTemperatureUnit {
    AMIWEATHER_TEMPERATURE_CELSIUS = 0,
    AMIWEATHER_TEMPERATURE_FAHRENHEIT
};

enum AmiWeatherWindUnit {
    AMIWEATHER_WIND_KMH = 0,
    AMIWEATHER_WIND_MPH
};

struct AmiWeatherPreferences {
    unsigned int format_version;
    struct AmiWeatherLocation saved_locations[AMIWEATHER_MAX_SAVED_LOCATIONS];
    size_t saved_location_count;
    size_t default_location_index;
    int has_default_location;
    enum AmiWeatherTemperatureUnit temperature_unit;
    enum AmiWeatherWindUnit wind_unit;
};

enum AmiWeatherPreferencesError {
    AMIWEATHER_PREFS_OK = 0,
    AMIWEATHER_PREFS_NOT_FOUND,
    AMIWEATHER_PREFS_INVALID,
    AMIWEATHER_PREFS_UNSUPPORTED_VERSION,
    AMIWEATHER_PREFS_IO_ERROR,
    AMIWEATHER_PREFS_FULL,
    AMIWEATHER_PREFS_DUPLICATE
};

void amiweather_preferences_default(struct AmiWeatherPreferences *preferences);
const char *amiweather_preferences_error_string(enum AmiWeatherPreferencesError error);
enum AmiWeatherPreferencesError amiweather_preferences_parse(
    const char *data, size_t size, struct AmiWeatherPreferences *preferences);
enum AmiWeatherPreferencesError amiweather_preferences_serialize(
    const struct AmiWeatherPreferences *preferences,
    char *buffer, size_t size, size_t *written);
enum AmiWeatherPreferencesError amiweather_preferences_load_file(
    const char *path, struct AmiWeatherPreferences *preferences);
enum AmiWeatherPreferencesError amiweather_preferences_save_file(
    const char *path, struct AmiWeatherPreferences *preferences);
int amiweather_location_equal(const struct AmiWeatherLocation *left,
                              const struct AmiWeatherLocation *right);
enum AmiWeatherPreferencesError amiweather_preferences_add_location(
    struct AmiWeatherPreferences *preferences,
    const struct AmiWeatherLocation *location);
enum AmiWeatherPreferencesError amiweather_preferences_remove_location(
    struct AmiWeatherPreferences *preferences, size_t index);
int amiweather_preferences_set_default(struct AmiWeatherPreferences *preferences,
                                       size_t index);
const char *amiweather_temperature_unit_query(enum AmiWeatherTemperatureUnit unit);
const char *amiweather_temperature_unit_label(enum AmiWeatherTemperatureUnit unit);
const char *amiweather_wind_unit_query(enum AmiWeatherWindUnit unit);
const char *amiweather_wind_unit_label(enum AmiWeatherWindUnit unit);

#endif
