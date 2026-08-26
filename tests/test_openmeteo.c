#include <amiweather/openmeteo.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
    ++failures; } } while (0)

struct WmoExpectation {
    long code;
    const char *description;
};

static void test_wmo_descriptions(void)
{
    static const struct WmoExpectation documented[] = {
        { 0, "Clear sky" }, { 1, "Mainly clear" },
        { 2, "Partly cloudy" }, { 3, "Overcast" },
        { 45, "Fog" }, { 48, "Rime fog" },
        { 51, "Light drizzle" }, { 53, "Drizzle" },
        { 55, "Heavy drizzle" }, { 56, "Light freezing drizzle" },
        { 57, "Freezing drizzle" }, { 61, "Light rain" },
        { 63, "Rain" }, { 65, "Heavy rain" },
        { 66, "Light freezing rain" }, { 67, "Freezing rain" },
        { 71, "Light snow" }, { 73, "Snow" },
        { 75, "Heavy snow" }, { 77, "Snow grains" },
        { 80, "Light rain showers" }, { 81, "Rain showers" },
        { 82, "Heavy rain showers" }, { 85, "Light snow showers" },
        { 86, "Snow showers" }, { 95, "Thunderstorm" },
        { 96, "Thunderstorm, light hail" },
        { 99, "Thunderstorm with hail" }
    };
    size_t i;

    for (i = 0; i < sizeof(documented) / sizeof(documented[0]); ++i)
        CHECK(strcmp(amiweather_wmo_description(documented[i].code),
                     documented[i].description) == 0);
    CHECK(strcmp(amiweather_wmo_description(4), "Unknown") == 0);
    CHECK(strcmp(amiweather_wmo_description(-1), "Unknown") == 0);
    CHECK(strcmp(amiweather_wmo_description(100), "Unknown") == 0);
}

static void test_valid(void)
{
    static const char fixture[] =
        "{\"latitude\":59.91,\"current\":{"
        "\"time\":\"2026-08-26T12:00\","
        "\"temperature_2m\":17.25,\"apparent_temperature\":16.5,"
        "\"relative_humidity_2m\":73,\"weather_code\":3,"
        "\"wind_speed_10m\":12.75}}";
    struct AmiWeatherCurrent current;
    struct AmiWeatherResult result = amiweather_openmeteo_parse_current(
        fixture, sizeof(fixture) - 1, &current);
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(fabs(current.temperature_c - 17.25) < 0.001);
    CHECK(fabs(current.apparent_temperature_c - 16.5) < 0.001);
    CHECK(fabs(current.relative_humidity_percent - 73.0) < 0.001);
    CHECK(fabs(current.wind_speed_kmh - 12.75) < 0.001);
    CHECK(current.weather_code == 3);
    CHECK(strcmp(current.observation_time, "2026-08-26T12:00") == 0);
}
static void test_missing(void)
{
    static const char fixture[] =
        "{\"current\":{\"time\":\"2026-08-26T12:00\","
        "\"temperature_2m\":17,\"apparent_temperature\":16,"
        "\"relative_humidity_2m\":73,\"weather_code\":3}}";
    struct AmiWeatherCurrent current;
    struct AmiWeatherResult result = amiweather_openmeteo_parse_current(
        fixture, sizeof(fixture) - 1, &current);
    CHECK(result.error == AMIWEATHER_ERR_MISSING_FIELD);
    CHECK(result.diagnostic[0] != '\0');
}

static void test_malformed(void)
{
    static const char fixture[] = "{\"current\":{";
    struct AmiWeatherCurrent current;
    struct AmiWeatherResult result = amiweather_openmeteo_parse_current(
        fixture, sizeof(fixture) - 1, &current);
    CHECK(result.error == AMIWEATHER_ERR_JSON);
}

static void test_invalid(void)
{
    static const char fixture[] =
        "{\"current\":{\"time\":\"2026-08-26T12:00\","
        "\"temperature_2m\":17,\"apparent_temperature\":16,"
        "\"relative_humidity_2m\":101,\"weather_code\":3,"
        "\"wind_speed_10m\":2}}";
    struct AmiWeatherCurrent current;
    struct AmiWeatherResult result = amiweather_openmeteo_parse_current(
        fixture, sizeof(fixture) - 1, &current);
    CHECK(result.error == AMIWEATHER_ERR_INVALID_FIELD);
}

static struct AmiWeatherResult parse_locations(const char *fixture,
                                                struct AmiWeatherLocationList *list)
{
    return amiweather_openmeteo_parse_locations(fixture, strlen(fixture), list);
}

static void test_geocoding_valid_and_optional(void)
{
    static const char fixture[] = "{\"results\":["
        "{\"name\":\"Grimstad\",\"latitude\":58.34,\"longitude\":8.59,"
        "\"admin1\":\"Agder\",\"country\":\"Norway\",\"country_code\":\"NO\","
        "\"timezone\":\"Europe/Oslo\"},"
        "{\"name\":\"Bare Place\",\"latitude\":1,\"longitude\":2}]}";
    struct AmiWeatherLocationList list;
    struct AmiWeatherResult result = parse_locations(fixture, &list);
    char display[AMIWEATHER_LOCATION_DISPLAY_SIZE];
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(list.count == 2);
    CHECK(strcmp(list.locations[0].name, "Grimstad") == 0);
    CHECK(strcmp(list.locations[0].admin1, "Agder") == 0);
    CHECK(strcmp(list.locations[1].admin1, "") == 0);
    CHECK(strcmp(list.locations[1].country, "") == 0);
    amiweather_location_format(&list.locations[0], display, sizeof(display));
    CHECK(strcmp(display, "Grimstad, Agder, Norway") == 0);
}

static void test_geocoding_empty_and_malformed(void)
{
    struct AmiWeatherLocationList list;
    struct AmiWeatherResult result;
    result = parse_locations("{}", &list);
    CHECK(result.error == AMIWEATHER_ERR_NOT_FOUND);
    CHECK(list.count == 0);
    result = parse_locations("{", &list);
    CHECK(result.error == AMIWEATHER_ERR_JSON);
}

static void expect_location_error(const char *fixture, enum AmiWeatherError error)
{
    struct AmiWeatherLocationList list;
    struct AmiWeatherResult result = parse_locations(fixture, &list);
    CHECK(result.error == error);
    CHECK(list.count == 0);
}

static void test_geocoding_required_fields(void)
{
    expect_location_error("{\"results\":[{\"latitude\":1,\"longitude\":2}]}",
                          AMIWEATHER_ERR_MISSING_FIELD);
    expect_location_error("{\"results\":[{\"name\":\"X\",\"longitude\":2}]}",
                          AMIWEATHER_ERR_MISSING_FIELD);
    expect_location_error("{\"results\":[{\"name\":\"X\",\"latitude\":1}]}",
                          AMIWEATHER_ERR_MISSING_FIELD);
    expect_location_error("{\"results\":[{\"name\":\"X\",\"latitude\":91,\"longitude\":2}]}",
                          AMIWEATHER_ERR_INVALID_FIELD);
    expect_location_error("{\"results\":[{\"name\":\"X\",\"latitude\":1,\"longitude\":181}]}",
                          AMIWEATHER_ERR_INVALID_FIELD);
}

static void test_geocoding_bounds(void)
{
    static const char fixture[] = "{\"results\":["
        "{\"name\":\"0\",\"latitude\":0,\"longitude\":0},"
        "{\"name\":\"1\",\"latitude\":1,\"longitude\":1},"
        "{\"name\":\"2\",\"latitude\":2,\"longitude\":2},"
        "{\"name\":\"3\",\"latitude\":3,\"longitude\":3},"
        "{\"name\":\"4\",\"latitude\":4,\"longitude\":4},"
        "{\"name\":\"5\",\"latitude\":5,\"longitude\":5},"
        "{\"name\":\"6\",\"latitude\":6,\"longitude\":6},"
        "{\"name\":\"7\",\"latitude\":7,\"longitude\":7},"
        "{\"name\":\"8\",\"latitude\":8,\"longitude\":8}]}";
    struct AmiWeatherLocationList list;
    struct AmiWeatherResult result = parse_locations(fixture, &list);
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(list.count == AMIWEATHER_LOCATION_MAX_RESULTS);
    expect_location_error("{\"results\":[{\"name\":\"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\",\"latitude\":1,\"longitude\":2}]}",
                          AMIWEATHER_ERR_INVALID_FIELD);
}

static void test_default_location(void)
{
    struct AmiWeatherLocation location;
    amiweather_location_default(&location);
    CHECK(strcmp(location.name, "Oslo") == 0);
    CHECK(fabs(location.latitude - 59.9139) < 0.00001);
    CHECK(fabs(location.longitude - 10.7522) < 0.00001);
    CHECK(strcmp(location.timezone, "Europe/Oslo") == 0);
}

static void test_empty_search_is_local_error(void)
{
    struct AmiWeatherLocationList list;
    struct AmiWeatherResult result = amiweather_geocode_search("  \t", &list);
    CHECK(result.error == AMIWEATHER_ERR_ARGUMENT);
}

static struct AmiWeatherResult parse_daily_object(
    const char *daily, struct AmiWeatherReport *report)
{
    char fixture[4096];
    int length = snprintf(fixture, sizeof(fixture),
        "{\"current\":{\"time\":\"2026-08-26T12:00\","
        "\"temperature_2m\":17.25,\"apparent_temperature\":16.5,"
        "\"relative_humidity_2m\":73,\"weather_code\":3,"
        "\"wind_speed_10m\":12.75},\"daily\":%s}", daily);
    CHECK(length > 0 && (size_t)length < sizeof(fixture));
    return amiweather_openmeteo_parse_report(fixture, (size_t)length, report);
}

static void expect_daily_error(const char *daily, enum AmiWeatherError error)
{
    struct AmiWeatherReport report;
    struct AmiWeatherResult result = parse_daily_object(daily, &report);
    CHECK(result.error == error);
}

static void test_forecast_valid_and_precipitation(void)
{
    static const char daily[] = "{"
        "\"time\":[\"2026-08-26\",\"2026-08-27\",\"2026-08-28\",\"2026-08-29\",\"2026-08-30\"],"
        "\"weather_code\":[2,63,4,0,81],"
        "\"temperature_2m_max\":[25.1,21.4,20.2,22,19.3],"
        "\"temperature_2m_min\":[17.2,15,-2.5,13.8,15.1],"
        "\"precipitation_probability_max\":[0,100,null,20,80]}";
    struct AmiWeatherReport report;
    struct AmiWeatherResult result = parse_daily_object(daily, &report);
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(report.forecast.count == 5);
    CHECK(fabs(report.forecast.days[0].temperature_min_c - 17.2) < 0.001);
    CHECK(fabs(report.forecast.days[0].temperature_max_c - 25.1) < 0.001);
    CHECK(report.forecast.days[0].has_precipitation_probability);
    CHECK(report.forecast.days[0].precipitation_probability == 0);
    CHECK(report.forecast.days[1].precipitation_probability == 100);
    CHECK(!report.forecast.days[2].has_precipitation_probability);
    CHECK(strcmp(amiweather_wmo_description(report.forecast.days[2].weather_code),
                 "Unknown") == 0);
    CHECK(fabs(report.forecast.days[2].temperature_min_c - -2.5) < 0.001);
}

static void test_forecast_bounds_and_optional_precipitation(void)
{
    struct AmiWeatherReport report;
    struct AmiWeatherResult result;
    result = parse_daily_object(
        "{\"time\":[\"2026-08-26\",\"2026-08-27\"],"
        "\"weather_code\":[0,1],\"temperature_2m_max\":[2,3],"
        "\"temperature_2m_min\":[1,2]}", &report);
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(report.forecast.count == 2);
    CHECK(!report.forecast.days[0].has_precipitation_probability);
    result = parse_daily_object(
        "{\"time\":[\"2026-08-26\",\"2026-08-27\",\"2026-08-28\",\"2026-08-29\",\"2026-08-30\",\"2026-08-31\"],"
        "\"weather_code\":[0,1,2,3,45,48],"
        "\"temperature_2m_max\":[2,3,4,5,6,7],"
        "\"temperature_2m_min\":[1,2,3,4,5,6]}", &report);
    CHECK(result.error == AMIWEATHER_OK);
    CHECK(report.forecast.count == AMIWEATHER_FORECAST_DAYS);
    CHECK(strcmp(report.forecast.days[4].date, "2026-08-30") == 0);
}

static void test_forecast_required_arrays(void)
{
    expect_daily_error("{\"time\":[],\"weather_code\":[],"
        "\"temperature_2m_max\":[],\"temperature_2m_min\":[]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"weather_code\":[0],\"temperature_2m_max\":[2],"
        "\"temperature_2m_min\":[1]}", AMIWEATHER_ERR_MISSING_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"temperature_2m_max\":[2],"
        "\"temperature_2m_min\":[1]}", AMIWEATHER_ERR_MISSING_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_min\":[1]}", AMIWEATHER_ERR_MISSING_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2]}", AMIWEATHER_ERR_MISSING_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\",\"2026-08-27\"],"
        "\"weather_code\":[0],\"temperature_2m_max\":[2,3],"
        "\"temperature_2m_min\":[1,2]}", AMIWEATHER_ERR_INVALID_FIELD);
}

static void test_forecast_invalid_entries(void)
{
    expect_daily_error("{\"time\":[null],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-02-30\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[null],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[100],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[null],\"temperature_2m_min\":[1]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[null]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[101],\"temperature_2m_min\":[1]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[1],\"temperature_2m_min\":[2]}",
        AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1],"
        "\"precipitation_probability_max\":[-1]}", AMIWEATHER_ERR_INVALID_FIELD);
    expect_daily_error("{\"time\":[\"2026-08-26\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1],"
        "\"precipitation_probability_max\":[101]}", AMIWEATHER_ERR_INVALID_FIELD);
}

static void test_report_commit_is_atomic(void)
{
    struct AmiWeatherReport report;
    struct AmiWeatherResult result;
    memset(&report, 0, sizeof(report));
    report.current.weather_code = 63;
    report.forecast.count = 4;
    result = parse_daily_object(
        "{\"time\":[\"bad-date\"],\"weather_code\":[0],"
        "\"temperature_2m_max\":[2],\"temperature_2m_min\":[1]}", &report);
    CHECK(result.error == AMIWEATHER_ERR_INVALID_FIELD);
    CHECK(report.current.weather_code == 63);
    CHECK(report.forecast.count == 4);
}

static void test_forecast_date_formatting(void)
{
    char display[AMIWEATHER_COMPACT_DATE_SIZE];
    CHECK(amiweather_format_forecast_date("2026-08-26", display,
                                          sizeof(display)));
    CHECK(strcmp(display, "Wed 26") == 0);
    CHECK(amiweather_format_forecast_date("2024-02-29", display,
                                          sizeof(display)));
    CHECK(strcmp(display, "Thu 29") == 0);
    CHECK(amiweather_format_forecast_date("2026-08-31", display,
                                          sizeof(display)));
    CHECK(strcmp(display, "Mon 31") == 0);
    CHECK(amiweather_format_forecast_date("2027-01-01", display,
                                          sizeof(display)));
    CHECK(strcmp(display, "Fri 01") == 0);
    CHECK(!amiweather_format_forecast_date("2026-02-29", display,
                                           sizeof(display)));
}

static void test_compact_descriptions(void)
{
    CHECK(strcmp(amiweather_wmo_compact_description(96), "T-storm / hail") == 0);
    CHECK(strcmp(amiweather_wmo_compact_description(2), "Partly cloudy") == 0);
    CHECK(strcmp(amiweather_wmo_compact_description(4), "Unknown") == 0);
}

int main(void)
{
    test_wmo_descriptions();
    test_valid();
    test_missing();
    test_malformed();
    test_invalid();
    test_geocoding_valid_and_optional();
    test_geocoding_empty_and_malformed();
    test_geocoding_required_fields();
    test_geocoding_bounds();
    test_default_location();
    test_empty_search_is_local_error();
    test_forecast_valid_and_precipitation();
    test_forecast_bounds_and_optional_precipitation();
    test_forecast_required_arrays();
    test_forecast_invalid_entries();
    test_report_commit_is_atomic();
    test_forecast_date_formatting();
    test_compact_descriptions();
    if (failures) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    puts("AmiWeather deterministic tests passed");
    return 0;
}
