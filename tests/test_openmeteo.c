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

int main(void)
{
    test_wmo_descriptions();
    test_valid();
    test_missing();
    test_malformed();
    test_invalid();
    if (failures) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    puts("AmiWeather deterministic tests passed");
    return 0;
}
