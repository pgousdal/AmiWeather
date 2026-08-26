#include <amiweather/openmeteo.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
    ++failures; } } while (0)

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
    test_valid();
    test_missing();
    test_malformed();
    test_invalid();
    if (failures) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    puts("Open-Meteo fixture tests passed");
    return 0;
}
