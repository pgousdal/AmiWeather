#include <amiweather/arexx.h>

#include <stdio.h>
#include <string.h>

static int failures, search_count = 1, search_fails, saved, save_fails;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

static struct AmiWeatherLocation grimstad(void)
{
    struct AmiWeatherLocation l;
    memset(&l, 0, sizeof(l));
    snprintf(l.name, sizeof(l.name), "%s", "Grimstad");
    snprintf(l.country, sizeof(l.country), "%s", "Norway");
    snprintf(l.country_code, sizeof(l.country_code), "%s", "NO");
    snprintf(l.timezone, sizeof(l.timezone), "%s", "Europe/Oslo");
    l.latitude = 58.34; l.longitude = 8.59; return l;
}

static struct AmiWeatherResult mock_search(const char *query,
    struct AmiWeatherLocationList *locations)
{
    struct AmiWeatherResult r;
    (void)query; memset(&r, 0, sizeof(r)); memset(locations, 0, sizeof(*locations));
    if (search_fails) { r.error = AMIWEATHER_ERR_NETWORK; return r; }
    locations->count = (size_t)search_count;
    if (locations->count) locations->locations[0] = grimstad();
    if (locations->count > 1) { locations->locations[1] = grimstad(); snprintf(locations->locations[1].name, sizeof(locations->locations[1].name), "%s", "Grimstad East"); }
    return r;
}

static struct AmiWeatherResult mock_fetch(const struct AmiWeatherLocation *location,
    enum AmiWeatherTemperatureUnit temperature, enum AmiWeatherWindUnit wind,
    struct AmiWeatherReport *report)
{
    struct AmiWeatherResult r;
    (void)location; memset(&r, 0, sizeof(r)); memset(report, 0, sizeof(*report));
    report->temperature_unit = temperature; report->wind_unit = wind;
    report->current.temperature_c = 12.3; report->current.relative_humidity_percent = 67;
    report->current.wind_speed_kmh = 4.5; report->current.weather_code = 0; return r;
}

static enum AmiWeatherPreferencesError mock_save(struct AmiWeatherPreferences *p)
{ (void)p; ++saved; return save_fails ? AMIWEATHER_PREFS_IO_ERROR : AMIWEATHER_PREFS_OK; }

static int command(const char *text, struct AmiWeatherARexxState *state,
    const struct AmiWeatherARexxOperations *ops, char *result)
{ return amiweather_arexx_command(text, strlen(text), state, ops, result, AMIWEATHER_AREXX_RESULT_SIZE); }

int main(void)
{
    struct AmiWeatherPreferences preferences;
    struct AmiWeatherLocation location;
    struct AmiWeatherReport report;
    struct AmiWeatherResult last;
    struct AmiWeatherARexxState state;
    struct AmiWeatherARexxOperations ops = { mock_search, mock_fetch, mock_save };
    char result[AMIWEATHER_AREXX_RESULT_SIZE], oversized[AMIWEATHER_AREXX_COMMAND_SIZE + 1];
    int has_location = 0, has_report = 0, refreshing = 0;
    amiweather_preferences_default(&preferences); memset(&location, 0, sizeof(location));
    memset(&report, 0, sizeof(report)); memset(&last, 0, sizeof(last));
    state.preferences=&preferences; state.location=&location; state.report=&report;
    state.has_location=&has_location; state.has_report=&has_report;
    state.refresh_active=&refreshing; state.last_result=&last;
    CHECK(command("VERSION", &state, &ops, result) == 0 && strstr(result, "AmiWeather") != NULL);
    CHECK(command("GET LOCATION", &state, &ops, result) == AMIWEATHER_AREXX_NO_LOCATION);
    CHECK(command("GET TEMPERATURE", &state, &ops, result) == AMIWEATHER_AREXX_NO_REPORT);
    CHECK(command("REFRESH", &state, &ops, result) == AMIWEATHER_AREXX_NO_LOCATION);
    CHECK(command("STATUS", &state, &ops, result) == 0 && strstr(result, "WEATHER=UNAVAILABLE") != NULL);
    CHECK(command("SET TEMPUNIT F", &state, &ops, result) == 0);
    CHECK(command("SET WINDUNIT MPH", &state, &ops, result) == 0);
    search_count = 2;
    CHECK(command("LOCATION Grimstad", &state, &ops, result) == AMIWEATHER_AREXX_AMBIGUOUS_LOCATION);
    CHECK(!has_location && !has_report);
    search_count = 1;
    search_fails = 1;
    CHECK(command("LOCATION Grimstad", &state, &ops, result) == AMIWEATHER_AREXX_GEOCODING_FAILED);
    search_fails = 0;
    CHECK(command("LOCATION Grimstad", &state, &ops, result) == 0);
    CHECK(has_location && has_report && strcmp(location.name, "Grimstad") == 0);
    CHECK(command("GET LOCATION", &state, &ops, result) == 0 && strstr(result, "Grimstad") != NULL);
    CHECK(command("GET TEMPERATURE", &state, &ops, result) == 0 && strcmp(result, "12.3 F") == 0);
    CHECK(command("GET CONDITION", &state, &ops, result) == 0 && result[0]);
    CHECK(command("GET HUMIDITY", &state, &ops, result) == 0 && strcmp(result, "67 %") == 0);
    CHECK(command("GET WIND", &state, &ops, result) == 0 && strcmp(result, "4.5 mph") == 0);
    CHECK(command("REFRESH", &state, &ops, result) == 0);
    CHECK(command("SET TEMPUNIT C", &state, &ops, result) == 0);
    CHECK(command("SET TEMPUNIT F", &state, &ops, result) == 0);
    CHECK(command("SET WINDUNIT KMH", &state, &ops, result) == 0);
    CHECK(command("SET WINDUNIT MPH", &state, &ops, result) == 0);
    CHECK(command("SETDEFAULT", &state, &ops, result) == 0 && saved == 1);
    CHECK(preferences.has_default_location);
    CHECK(command("SAVE", &state, &ops, result) == 0 && saved == 2);
    save_fails = 1;
    CHECK(command("SAVE", &state, &ops, result) == AMIWEATHER_AREXX_SAVE_FAILED);
    save_fails = 0;
    CHECK(command("", &state, &ops, result) == AMIWEATHER_AREXX_INVALID_ARGUMENTS);
    CHECK(command("LOCATION ", &state, &ops, result) == AMIWEATHER_AREXX_INVALID_ARGUMENTS);
    CHECK(command("SET TEMPUNIT X", &state, &ops, result) == AMIWEATHER_AREXX_INVALID_ARGUMENTS);
    CHECK(command("BOGUS", &state, &ops, result) == AMIWEATHER_AREXX_UNKNOWN_COMMAND);
    memset(oversized, 'A', sizeof(oversized)); oversized[sizeof(oversized)-1] = '\0';
    CHECK(amiweather_arexx_command(oversized, sizeof(oversized)-1, &state, &ops,
        result, sizeof(result)) == AMIWEATHER_AREXX_INVALID_ARGUMENTS);
    if (failures) return 1;
    puts("AmiWeather ARexx command tests passed"); return 0;
}
