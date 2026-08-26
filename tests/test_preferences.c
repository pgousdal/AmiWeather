#include <amiweather/openmeteo.h>
#include <amiweather/preferences.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int failures;
#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
    ++failures; } } while (0)

static struct AmiWeatherLocation location_make(const char *name, double latitude,
                                                double longitude)
{
    struct AmiWeatherLocation location;
    memset(&location, 0, sizeof(location));
    snprintf(location.name, sizeof(location.name), "%s", name);
    snprintf(location.admin1, sizeof(location.admin1), "%s", "Region");
    snprintf(location.country, sizeof(location.country), "%s", "Norway");
    snprintf(location.country_code, sizeof(location.country_code), "%s", "NO");
    snprintf(location.timezone, sizeof(location.timezone), "%s", "Europe/Oslo");
    location.latitude = latitude;
    location.longitude = longitude;
    return location;
}

static void test_defaults_and_units(void)
{
    struct AmiWeatherPreferences preferences;
    amiweather_preferences_default(&preferences);
    CHECK(preferences.format_version == 1);
    CHECK(preferences.saved_location_count == 0);
    CHECK(!preferences.has_default_location);
    CHECK(preferences.temperature_unit == AMIWEATHER_TEMPERATURE_CELSIUS);
    CHECK(preferences.wind_unit == AMIWEATHER_WIND_KMH);
    CHECK(strcmp(amiweather_temperature_unit_query(AMIWEATHER_TEMPERATURE_CELSIUS), "celsius") == 0);
    CHECK(strcmp(amiweather_temperature_unit_query(AMIWEATHER_TEMPERATURE_FAHRENHEIT), "fahrenheit") == 0);
    CHECK(strcmp(amiweather_temperature_unit_label(AMIWEATHER_TEMPERATURE_FAHRENHEIT), "F") == 0);
    CHECK(strcmp(amiweather_wind_unit_query(AMIWEATHER_WIND_KMH), "kmh") == 0);
    CHECK(strcmp(amiweather_wind_unit_query(AMIWEATHER_WIND_MPH), "mph") == 0);
    CHECK(strcmp(amiweather_wind_unit_label(AMIWEATHER_WIND_KMH), "km/h") == 0);
}

static void test_parse_formats(void)
{
    static const char valid_lf[] =
        "version=1\ntemperature=fahrenheit\nwind=mph\ndefault=0\n"
        "unknown=future\n"
        "location=Oslo|Oslo|Norway|NO|59.913900|10.752200|Europe/Oslo\n";
    static const char valid_crlf[] =
        "version=1\r\ntemperature=celsius\r\nwind=kmh\r\n";
    struct AmiWeatherPreferences preferences;
    enum AmiWeatherPreferencesError result;
    result = amiweather_preferences_parse(valid_lf, sizeof(valid_lf) - 1,
                                           &preferences);
    CHECK(result == AMIWEATHER_PREFS_OK);
    CHECK(preferences.saved_location_count == 1);
    CHECK(preferences.has_default_location && preferences.default_location_index == 0);
    CHECK(preferences.temperature_unit == AMIWEATHER_TEMPERATURE_FAHRENHEIT);
    CHECK(preferences.wind_unit == AMIWEATHER_WIND_MPH);
    result = amiweather_preferences_parse(valid_crlf, sizeof(valid_crlf) - 1,
                                           &preferences);
    CHECK(result == AMIWEATHER_PREFS_OK);
    CHECK(preferences.saved_location_count == 0);
}

static void expect_parse_error(const char *text, enum AmiWeatherPreferencesError error)
{
    struct AmiWeatherPreferences preferences;
    enum AmiWeatherPreferencesError result;
    amiweather_preferences_default(&preferences);
    preferences.saved_location_count = 7;
    result = amiweather_preferences_parse(text, strlen(text), &preferences);
    CHECK(result == error);
    CHECK(preferences.saved_location_count == 7);
}

static void test_parser_errors(void)
{
    char oversized[AMIWEATHER_PREFERENCES_LINE_SIZE + 20];
    memset(oversized, 'x', sizeof(oversized)); oversized[0] = 'x';
    oversized[sizeof(oversized) - 2] = '\n'; oversized[sizeof(oversized) - 1] = '\0';
    expect_parse_error("temperature=celsius\n", AMIWEATHER_PREFS_INVALID);
    expect_parse_error("version=2\n", AMIWEATHER_PREFS_UNSUPPORTED_VERSION);
    expect_parse_error("version=1", AMIWEATHER_PREFS_INVALID);
    expect_parse_error("version=1\nlocation=Bad\n", AMIWEATHER_PREFS_INVALID);
    expect_parse_error("version=1\nlocation=X|R|C|NO||2|Zone\n", AMIWEATHER_PREFS_INVALID);
    expect_parse_error("version=1\nlocation=X|R|C|NO|1||Zone\n", AMIWEATHER_PREFS_INVALID);
    expect_parse_error("version=1\nlocation=X|R|C|NO|91|2|Zone\n", AMIWEATHER_PREFS_INVALID);
    expect_parse_error("version=1\nlocation=X|R|C|NO|1|181|Zone\n", AMIWEATHER_PREFS_INVALID);
    expect_parse_error(oversized, AMIWEATHER_PREFS_INVALID);
    {
        char field[180]; char file[300];
        memset(field, 'a', sizeof(field) - 1); field[sizeof(field) - 1] = '\0';
        snprintf(file, sizeof(file), "version=1\nlocation=%s|R|C|NO|1|2|Zone\n", field);
        expect_parse_error(file, AMIWEATHER_PREFS_INVALID);
    }
}

static void test_saved_location_operations(void)
{
    struct AmiWeatherPreferences preferences;
    struct AmiWeatherLocation locations[9];
    size_t i;
    amiweather_preferences_default(&preferences);
    for (i = 0; i < 9; ++i) {
        char name[16]; snprintf(name, sizeof(name), "Place%lu", (unsigned long)i);
        locations[i] = location_make(name, 50.0 + i, 5.0 + i);
    }
    CHECK(amiweather_preferences_add_location(&preferences, &locations[0]) == AMIWEATHER_PREFS_OK);
    CHECK(amiweather_preferences_add_location(&preferences, &locations[0]) == AMIWEATHER_PREFS_DUPLICATE);
    for (i = 1; i < 8; ++i)
        CHECK(amiweather_preferences_add_location(&preferences, &locations[i]) == AMIWEATHER_PREFS_OK);
    CHECK(preferences.saved_location_count == 8);
    CHECK(amiweather_preferences_add_location(&preferences, &locations[8]) == AMIWEATHER_PREFS_FULL);
    CHECK(amiweather_preferences_set_default(&preferences, 5));
    CHECK(amiweather_preferences_remove_location(&preferences, 0) == AMIWEATHER_PREFS_OK);
    CHECK(preferences.default_location_index == 4);
    CHECK(amiweather_preferences_remove_location(&preferences, 2) == AMIWEATHER_PREFS_OK);
    CHECK(preferences.default_location_index == 3);
    CHECK(amiweather_preferences_remove_location(&preferences, 3) == AMIWEATHER_PREFS_OK);
    CHECK(preferences.default_location_index == 0);
    while (preferences.saved_location_count > 1)
        CHECK(amiweather_preferences_remove_location(&preferences,
              preferences.saved_location_count - 1) == AMIWEATHER_PREFS_OK);
    CHECK(amiweather_preferences_remove_location(&preferences, 0) == AMIWEATHER_PREFS_OK);
    CHECK(!preferences.has_default_location && preferences.saved_location_count == 0);
}

static void test_duplicate_file_and_round_trip(void)
{
    struct AmiWeatherPreferences source, parsed;
    struct AmiWeatherLocation oslo = location_make("Oslo", 59.9139, 10.7522);
    char serialized[8192]; size_t written;
    amiweather_preferences_default(&source);
    source.temperature_unit = AMIWEATHER_TEMPERATURE_FAHRENHEIT;
    source.wind_unit = AMIWEATHER_WIND_MPH;
    CHECK(amiweather_preferences_add_location(&source, &oslo) == AMIWEATHER_PREFS_OK);
    CHECK(amiweather_preferences_set_default(&source, 0));
    CHECK(amiweather_preferences_serialize(&source, serialized, sizeof(serialized),
                                           &written) == AMIWEATHER_PREFS_OK);
    CHECK(amiweather_preferences_parse(serialized, written, &parsed) == AMIWEATHER_PREFS_OK);
    CHECK(parsed.saved_location_count == 1);
    CHECK(amiweather_location_equal(&source.saved_locations[0], &parsed.saved_locations[0]));
    CHECK(parsed.temperature_unit == source.temperature_unit && parsed.wind_unit == source.wind_unit);
    expect_parse_error(
        "version=1\nlocation=X|R|C|NO|1|2|Zone\nlocation=X|R|C|NO|1|2|Zone\n",
        AMIWEATHER_PREFS_DUPLICATE);
}

static void test_eight_and_ninth_in_format(void)
{
    struct AmiWeatherPreferences source, parsed;
    char serialized[8192];
    size_t written, i;
    amiweather_preferences_default(&source);
    for (i = 0; i < AMIWEATHER_MAX_SAVED_LOCATIONS; ++i) {
        char name[16];
        struct AmiWeatherLocation location;
        snprintf(name, sizeof(name), "Saved%lu", (unsigned long)i);
        location = location_make(name, 50.0 + i, 5.0 + i);
        CHECK(amiweather_preferences_add_location(&source, &location) == AMIWEATHER_PREFS_OK);
    }
    CHECK(amiweather_preferences_serialize(&source, serialized, sizeof(serialized),
                                           &written) == AMIWEATHER_PREFS_OK);
    CHECK(amiweather_preferences_parse(serialized, written, &parsed) == AMIWEATHER_PREFS_OK);
    CHECK(parsed.saved_location_count == AMIWEATHER_MAX_SAVED_LOCATIONS);
    snprintf(serialized + written, sizeof(serialized) - written,
             "location=Ninth|R|Norway|NO|10|20|Europe/Oslo\n");
    expect_parse_error(serialized, AMIWEATHER_PREFS_FULL);
}

static void test_query_units(void)
{
    struct AmiWeatherLocation location;
    struct AmiWeatherResult result;
    char url[1024];
    amiweather_location_default(&location);
    result = amiweather_openmeteo_report_url(&location,
        AMIWEATHER_TEMPERATURE_CELSIUS, AMIWEATHER_WIND_KMH, url, sizeof(url));
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(strstr(url, "temperature_unit=celsius") != NULL);
    CHECK(strstr(url, "wind_speed_unit=kmh") != NULL);
    CHECK(strstr(url, "current=") != NULL && strstr(url, "daily=") != NULL);
    CHECK(strstr(url, "hourly=") != NULL);
    result = amiweather_openmeteo_report_url(&location,
        AMIWEATHER_TEMPERATURE_FAHRENHEIT, AMIWEATHER_WIND_MPH, url, sizeof(url));
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(strstr(url, "temperature_unit=fahrenheit") != NULL);
    CHECK(strstr(url, "wind_speed_unit=mph") != NULL);
}

static void test_explicit_file_io(void)
{
    char path[128];
    struct AmiWeatherPreferences source, loaded;
    snprintf(path, sizeof(path), "/tmp/amiweather-prefs-%ld", (long)getpid());
    remove(path); remove("/tmp/amiweather-prefs-impossible/preferences");
    amiweather_preferences_default(&source);
    CHECK(amiweather_preferences_load_file(path, &loaded) == AMIWEATHER_PREFS_NOT_FOUND);
    CHECK(amiweather_preferences_save_file(path, &source) == AMIWEATHER_PREFS_OK);
    CHECK(amiweather_preferences_load_file(path, &loaded) == AMIWEATHER_PREFS_OK);
    {
        FILE *file = fopen(path, "wb");
        CHECK(file != NULL);
        if (file) {
            CHECK(fputs("version=1", file) >= 0);
            CHECK(fclose(file) == 0);
        }
        loaded.saved_location_count = 6;
        CHECK(amiweather_preferences_load_file(path, &loaded) == AMIWEATHER_PREFS_INVALID);
        CHECK(loaded.saved_location_count == 6);
    }
    CHECK(amiweather_preferences_save_file("/tmp/amiweather-prefs-impossible/preferences",
                                           &source) == AMIWEATHER_PREFS_IO_ERROR);
    remove(path);
}

int main(void)
{
    test_defaults_and_units();
    test_parse_formats();
    test_parser_errors();
    test_saved_location_operations();
    test_duplicate_file_and_round_trip();
    test_eight_and_ninth_in_format();
    test_query_units();
    test_explicit_file_io();
    if (failures) { fprintf(stderr, "%d preference test(s) failed\n", failures); return 1; }
    puts("AmiWeather preference tests passed");
    return 0;
}
