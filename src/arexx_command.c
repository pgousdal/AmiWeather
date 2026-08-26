#include <amiweather/arexx.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void set_result(char *result, size_t size, const char *text)
{
    if (result && size)
        snprintf(result, size, "%s", text ? text : "");
}

static void tenths(char *out, size_t size, double value)
{
    long v = value >= 0.0 ? (long)(value * 10.0 + 0.5) :
        (long)(value * 10.0 - 0.5);
    unsigned long m = v < 0 ? (unsigned long)(-(v + 1L)) + 1UL :
        (unsigned long)v;
    snprintf(out, size, "%s%lu.%lu", v < 0 ? "-" : "", m / 10UL, m % 10UL);
}

static char *trim(char *text)
{
    char *end;
    while (*text && isspace((unsigned char)*text)) ++text;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

static int same(const char *left, const char *right)
{
    while (*left && *right) {
        if (toupper((unsigned char)*left) != toupper((unsigned char)*right))
            return 0;
        ++left; ++right;
    }
    return *left == '\0' && *right == '\0';
}

static int starts(const char *text, const char *prefix)
{
    while (*prefix) {
        if (!*text || toupper((unsigned char)*text) !=
            toupper((unsigned char)*prefix)) return 0;
        ++text; ++prefix;
    }
    return 1;
}

static int persist(struct AmiWeatherARexxState *state,
                   const struct AmiWeatherARexxOperations *operations,
                   char *result, size_t result_size)
{
    enum AmiWeatherPreferencesError error;
    if (!operations->save) {
        set_result(result, result_size, "UNAVAILABLE");
        return AMIWEATHER_AREXX_UNAVAILABLE;
    }
    error = operations->save(state->preferences);
    if (error != AMIWEATHER_PREFS_OK) {
        set_result(result, result_size, amiweather_preferences_error_string(error));
        return AMIWEATHER_AREXX_SAVE_FAILED;
    }
    set_result(result, result_size, "OK");
    return AMIWEATHER_AREXX_OK;
}

static int set_units(struct AmiWeatherARexxState *state,
    const struct AmiWeatherARexxOperations *operations,
    enum AmiWeatherTemperatureUnit temperature,
    enum AmiWeatherWindUnit wind, char *result, size_t result_size)
{
    if (*state->has_location && *state->has_report) {
        struct AmiWeatherReport updated;
        struct AmiWeatherResult fetched;
        if (!operations->fetch) { set_result(result, result_size, "UNAVAILABLE"); return AMIWEATHER_AREXX_UNAVAILABLE; }
        fetched = operations->fetch(state->location, temperature, wind, &updated);
        if (state->last_result) *state->last_result = fetched;
        if (fetched.error != AMIWEATHER_OK) {
            set_result(result, result_size, fetched.diagnostic[0] ? fetched.diagnostic : amiweather_error_string(fetched.error));
            return AMIWEATHER_AREXX_UNAVAILABLE;
        }
        *state->report = updated;
    }
    state->preferences->temperature_unit = temperature;
    state->preferences->wind_unit = wind;
    set_result(result, result_size, "OK");
    return AMIWEATHER_AREXX_OK;
}

int amiweather_arexx_command(const char *command, size_t command_length,
    struct AmiWeatherARexxState *state,
    const struct AmiWeatherARexxOperations *operations,
    char *result, size_t result_size)
{
    char input[AMIWEATHER_AREXX_COMMAND_SIZE], value[64];
    char *text;
    if (!result || !result_size || !state || !operations || !command) return AMIWEATHER_AREXX_INVALID_ARGUMENTS;
    result[0] = '\0';
    if (!command_length || command_length >= sizeof(input)) {
        set_result(result, result_size, "INVALID ARGUMENTS");
        return AMIWEATHER_AREXX_INVALID_ARGUMENTS;
    }
    memcpy(input, command, command_length); input[command_length] = '\0';
    text = trim(input);
    if (same(text, "VERSION")) { snprintf(result, result_size, "AmiWeather %s", AMIWEATHER_VERSION); return 0; }
    if (same(text, "STATUS")) {
        char place[AMIWEATHER_LOCATION_DISPLAY_SIZE];
        if (*state->has_location) amiweather_location_format(state->location, place, sizeof(place));
        else snprintf(place, sizeof(place), "%s", "NONE");
        snprintf(result, result_size, "WEATHER=%s LOCATION=%s STATE=%s",
            *state->has_report ? "AVAILABLE" : "UNAVAILABLE", place,
            state->refresh_active && *state->refresh_active ? "REFRESHING" :
            (state->last_result && state->last_result->error != AMIWEATHER_OK ? "ERROR" : "IDLE"));
        return 0;
    }
    if (same(text, "GET LOCATION")) {
        if (!*state->has_location) { set_result(result, result_size, "NO ACTIVE LOCATION"); return AMIWEATHER_AREXX_NO_LOCATION; }
        amiweather_location_format(state->location, result, result_size); return 0;
    }
    if (starts(text, "GET ")) {
        if (!*state->has_report) { set_result(result, result_size, "NO CURRENT WEATHER REPORT"); return AMIWEATHER_AREXX_NO_REPORT; }
        if (same(text, "GET TEMPERATURE")) { tenths(value, sizeof(value), state->report->current.temperature_c); snprintf(result, result_size, "%s %s", value, amiweather_temperature_unit_label(state->preferences->temperature_unit)); return 0; }
        if (same(text, "GET CONDITION")) { set_result(result, result_size, amiweather_wmo_description(state->report->current.weather_code)); return 0; }
        if (same(text, "GET HUMIDITY")) { snprintf(result, result_size, "%ld %%", (long)(state->report->current.relative_humidity_percent + 0.5)); return 0; }
        if (same(text, "GET WIND")) { tenths(value, sizeof(value), state->report->current.wind_speed_kmh); snprintf(result, result_size, "%s %s", value, amiweather_wind_unit_label(state->preferences->wind_unit)); return 0; }
    }
    if (same(text, "REFRESH")) {
        struct AmiWeatherReport updated; struct AmiWeatherResult fetched;
        if (!*state->has_location) { set_result(result, result_size, "NO ACTIVE LOCATION"); return AMIWEATHER_AREXX_NO_LOCATION; }
        if (!operations->fetch) { set_result(result, result_size, "UNAVAILABLE"); return AMIWEATHER_AREXX_UNAVAILABLE; }
        if (state->refresh_active) *state->refresh_active = 1;
        fetched = operations->fetch(state->location, state->preferences->temperature_unit, state->preferences->wind_unit, &updated);
        if (state->refresh_active) *state->refresh_active = 0;
        if (state->last_result) *state->last_result = fetched;
        if (fetched.error != AMIWEATHER_OK) { set_result(result, result_size, fetched.diagnostic[0] ? fetched.diagnostic : amiweather_error_string(fetched.error)); return AMIWEATHER_AREXX_UNAVAILABLE; }
        *state->report = updated; *state->has_report = 1; set_result(result, result_size, "OK"); return 0;
    }
    if (strlen(text) > 9 && starts(text, "LOCATION") &&
        isspace((unsigned char)text[8])) {
        struct AmiWeatherLocationList locations; struct AmiWeatherResult searched, fetched; struct AmiWeatherReport updated;
        char *query = trim(text + 9);
        if (!*query || strlen(query) >= AMIWEATHER_LOCATION_QUERY_SIZE) { set_result(result, result_size, "INVALID ARGUMENTS"); return AMIWEATHER_AREXX_INVALID_ARGUMENTS; }
        if (!operations->search || !operations->fetch) { set_result(result, result_size, "UNAVAILABLE"); return AMIWEATHER_AREXX_UNAVAILABLE; }
        searched = operations->search(query, &locations);
        if (searched.error != AMIWEATHER_OK) { set_result(result, result_size, searched.diagnostic[0] ? searched.diagnostic : amiweather_error_string(searched.error)); return AMIWEATHER_AREXX_GEOCODING_FAILED; }
        if (locations.count != 1) { set_result(result, result_size, locations.count ? "AMBIGUOUS LOCATION" : "LOCATION NOT FOUND"); return locations.count ? AMIWEATHER_AREXX_AMBIGUOUS_LOCATION : AMIWEATHER_AREXX_GEOCODING_FAILED; }
        fetched = operations->fetch(&locations.locations[0], state->preferences->temperature_unit, state->preferences->wind_unit, &updated);
        if (state->last_result) *state->last_result = fetched;
        if (fetched.error != AMIWEATHER_OK) { set_result(result, result_size, fetched.diagnostic[0] ? fetched.diagnostic : amiweather_error_string(fetched.error)); return AMIWEATHER_AREXX_UNAVAILABLE; }
        *state->location = locations.locations[0]; *state->has_location = 1; *state->report = updated; *state->has_report = 1;
        amiweather_location_format(state->location, result, result_size); return 0;
    }
    if (same(text, "LOCATION")) {
        set_result(result, result_size, "INVALID ARGUMENTS");
        return AMIWEATHER_AREXX_INVALID_ARGUMENTS;
    }
    if (same(text, "SETDEFAULT")) {
        struct AmiWeatherPreferences changed; enum AmiWeatherPreferencesError error;
        if (!*state->has_location) { set_result(result, result_size, "NO ACTIVE LOCATION"); return AMIWEATHER_AREXX_NO_LOCATION; }
        if (!operations->save) { set_result(result, result_size, "UNAVAILABLE"); return AMIWEATHER_AREXX_UNAVAILABLE; }
        changed = *state->preferences; error = amiweather_preferences_set_default_location(&changed, state->location);
        if (error == AMIWEATHER_PREFS_OK) error = operations->save(&changed);
        if (error != AMIWEATHER_PREFS_OK) { set_result(result, result_size, amiweather_preferences_error_string(error)); return AMIWEATHER_AREXX_SAVE_FAILED; }
        *state->preferences = changed; set_result(result, result_size, "OK"); return 0;
    }
    if (same(text, "SET TEMPUNIT C")) return set_units(state, operations, AMIWEATHER_TEMPERATURE_CELSIUS, state->preferences->wind_unit, result, result_size);
    if (same(text, "SET TEMPUNIT F")) return set_units(state, operations, AMIWEATHER_TEMPERATURE_FAHRENHEIT, state->preferences->wind_unit, result, result_size);
    if (same(text, "SET WINDUNIT KMH")) return set_units(state, operations, state->preferences->temperature_unit, AMIWEATHER_WIND_KMH, result, result_size);
    if (same(text, "SET WINDUNIT MPH")) return set_units(state, operations, state->preferences->temperature_unit, AMIWEATHER_WIND_MPH, result, result_size);
    if (same(text, "SAVE")) return persist(state, operations, result, result_size);
    if (starts(text, "SET ")) { set_result(result, result_size, "INVALID ARGUMENTS"); return AMIWEATHER_AREXX_INVALID_ARGUMENTS; }
    set_result(result, result_size, "UNKNOWN COMMAND");
    return AMIWEATHER_AREXX_UNKNOWN_COMMAND;
}
