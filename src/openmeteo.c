#include <amiweather/openmeteo.h>

#include <amiapi/amiapi.h>
#include <amiapi/errors.h>
#include <amiapi/json.h>
#include <amiapi/url.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPENMETEO_MAX_BODY (64UL * 1024UL)
#define OPENMETEO_TIMEOUT_SECONDS 20U
#define OPENMETEO_GEOCODING_LIMIT "8"

static struct AmiWeatherResult result_make(enum AmiWeatherError error,
                                            int status,
                                            const char *detail)
{
    struct AmiWeatherResult result;
    result.error = error;
    result.http_status = status;
    if (detail)
        snprintf(result.diagnostic, sizeof(result.diagnostic), "%s", detail);
    else
        result.diagnostic[0] = '\0';
    return result;
}

static int json_number(const struct AmiAPIJsonDoc *doc,
                       const struct AmiAPIJsonValue *object,
                       const char *name,
                       double *number)
{
    const struct AmiAPIJsonValue *value;
    const char *token;
    size_t length;
    char local[48];
    char *end;
    double parsed;

    value = AmiAPI_JSONObjectGet(doc, object, name);
    if (!value)
        return 0;
    if (AmiAPI_JSONType(value) != AMIAPI_JSON_NUMBER)
        return -1;
    if (AmiAPI_JSONNumberToken(doc, value, &token, &length) != AMIAPI_JSON_OK ||
        length == 0 || length >= sizeof(local))
        return -1;
    memcpy(local, token, length);
    local[length] = '\0';
    errno = 0;
    parsed = strtod(local, &end);
    if (errno != 0 || end != local + length)
        return -1;
    *number = parsed;
    return 1;
}

static int json_optional_number(const struct AmiAPIJsonDoc *doc,
                                const struct AmiAPIJsonValue *object,
                                const char *name, double *number)
{
    const struct AmiAPIJsonValue *value = AmiAPI_JSONObjectGet(doc, object, name);
    if (!value || AmiAPI_JSONType(value) == AMIAPI_JSON_NULL)
        return 0;
    return json_number(doc, object, name, number);
}

static int json_long(const struct AmiAPIJsonDoc *doc,
                     const struct AmiAPIJsonValue *object,
                     const char *name,
                     long *number)
{
    const struct AmiAPIJsonValue *value = AmiAPI_JSONObjectGet(doc, object, name);
    if (!value)
        return 0;
    if (AmiAPI_JSONType(value) != AMIAPI_JSON_NUMBER ||
        AmiAPI_JSONLong(doc, value, number) != AMIAPI_JSON_OK)
        return -1;
    return 1;
}

static int json_string(const struct AmiAPIJsonDoc *doc,
                       const struct AmiAPIJsonValue *object,
                       const char *name,
                       char *buffer,
                       size_t size)
{
    const struct AmiAPIJsonValue *value = AmiAPI_JSONObjectGet(doc, object, name);
    size_t required_size;
    if (!value)
        return 0;
    if (AmiAPI_JSONType(value) != AMIAPI_JSON_STRING ||
        AmiAPI_JSONStringCopy(doc, value, buffer, size, &required_size) != AMIAPI_JSON_OK)
        return -1;
    return 1;
}

static int json_optional_string(const struct AmiAPIJsonDoc *doc,
                                const struct AmiAPIJsonValue *object,
                                const char *name, char *buffer, size_t size)
{
    int rc = json_string(doc, object, name, buffer, size);
    if (rc == 0) {
        buffer[0] = '\0';
        return 1;
    }
    return rc;
}

static int json_array_number(const struct AmiAPIJsonDoc *doc,
                             const struct AmiAPIJsonValue *array,
                             size_t index, double *number)
{
    const struct AmiAPIJsonValue *value;
    const char *token;
    size_t length;
    char local[48];
    char *end;

    value = AmiAPI_JSONArrayGet(doc, array, index);
    if (!value || AmiAPI_JSONType(value) != AMIAPI_JSON_NUMBER)
        return 0;
    if (AmiAPI_JSONNumberToken(doc, value, &token, &length) != AMIAPI_JSON_OK ||
        length == 0 || length >= sizeof(local))
        return 0;
    memcpy(local, token, length);
    local[length] = '\0';
    errno = 0;
    *number = strtod(local, &end);
    return errno == 0 && end == local + length;
}

static int json_array_long(const struct AmiAPIJsonDoc *doc,
                           const struct AmiAPIJsonValue *array,
                           size_t index, long *number)
{
    const struct AmiAPIJsonValue *value = AmiAPI_JSONArrayGet(doc, array, index);
    return value && AmiAPI_JSONType(value) == AMIAPI_JSON_NUMBER &&
           AmiAPI_JSONLong(doc, value, number) == AMIAPI_JSON_OK;
}

static int json_array_string(const struct AmiAPIJsonDoc *doc,
                             const struct AmiAPIJsonValue *array,
                             size_t index, char *buffer, size_t size)
{
    const struct AmiAPIJsonValue *value = AmiAPI_JSONArrayGet(doc, array, index);
    size_t required;
    return value && AmiAPI_JSONType(value) == AMIAPI_JSON_STRING &&
           AmiAPI_JSONStringCopy(doc, value, buffer, size, &required) == AMIAPI_JSON_OK;
}

struct AmiWeatherResult amiweather_openmeteo_parse_locations(
    const void *json, size_t json_size, struct AmiWeatherLocationList *locations)
{
    struct AmiAPIJsonDoc *doc;
    const struct AmiAPIJsonValue *root;
    const struct AmiAPIJsonValue *array;
    enum AmiAPIJsonError json_error;
    size_t offset = 0;
    size_t count;
    size_t i;

    if (!json || !locations)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "null parser argument");
    memset(locations, 0, sizeof(*locations));
    doc = AmiAPI_JSONParse(json, json_size, &json_error, &offset);
    if (!doc) {
        char detail[96];
        snprintf(detail, sizeof(detail), "JSON error %d at byte %lu",
                 (int)json_error, (unsigned long)offset);
        return result_make(AMIWEATHER_ERR_JSON, 0, detail);
    }
    root = AmiAPI_JSONRoot(doc);
    array = root && AmiAPI_JSONType(root) == AMIAPI_JSON_OBJECT
        ? AmiAPI_JSONObjectGet(doc, root, "results") : NULL;
    if (!array) {
        AmiAPI_JSONFree(doc);
        return result_make(AMIWEATHER_ERR_NOT_FOUND, 0, "no matching locations");
    }
    if (AmiAPI_JSONType(array) != AMIAPI_JSON_ARRAY) {
        AmiAPI_JSONFree(doc);
        return result_make(AMIWEATHER_ERR_INVALID_FIELD, 0,
                           "geocoding results is not an array");
    }
    count = AmiAPI_JSONArraySize(array);
    if (count > AMIWEATHER_LOCATION_MAX_RESULTS)
        count = AMIWEATHER_LOCATION_MAX_RESULTS;
    for (i = 0; i < count; ++i) {
        const struct AmiAPIJsonValue *item = AmiAPI_JSONArrayGet(doc, array, i);
        struct AmiWeatherLocation *location = &locations->locations[i];
        int rc;
        if (!item || AmiAPI_JSONType(item) != AMIAPI_JSON_OBJECT)
            goto invalid;
        rc = json_string(doc, item, "name", location->name,
                         sizeof(location->name));
        if (rc < 0)
            goto invalid;
        if (rc == 0 || location->name[0] == '\0')
            goto required;
        rc = json_number(doc, item, "latitude", &location->latitude);
        if (rc < 0)
            goto invalid;
        if (rc == 0)
            goto required;
        rc = json_number(doc, item, "longitude", &location->longitude);
        if (rc < 0)
            goto invalid;
        if (rc == 0)
            goto required;
        if (location->latitude < -90.0 || location->latitude > 90.0 ||
            location->longitude < -180.0 || location->longitude > 180.0)
            goto invalid;
        if (json_optional_string(doc, item, "admin1", location->admin1,
                                 sizeof(location->admin1)) < 0 ||
            json_optional_string(doc, item, "country", location->country,
                                 sizeof(location->country)) < 0 ||
            json_optional_string(doc, item, "country_code", location->country_code,
                                 sizeof(location->country_code)) < 0 ||
            json_optional_string(doc, item, "timezone", location->timezone,
                                 sizeof(location->timezone)) < 0)
            goto invalid;
        ++locations->count;
    }
    AmiAPI_JSONFree(doc);
    if (locations->count == 0)
        return result_make(AMIWEATHER_ERR_NOT_FOUND, 0, "no matching locations");
    return result_make(AMIWEATHER_OK, 0, NULL);

required:
    AmiAPI_JSONFree(doc);
    memset(locations, 0, sizeof(*locations));
    return result_make(AMIWEATHER_ERR_MISSING_FIELD, 0,
                       "location requires name and coordinates");
invalid:
    AmiAPI_JSONFree(doc);
    memset(locations, 0, sizeof(*locations));
    return result_make(AMIWEATHER_ERR_INVALID_FIELD, 0,
                       "invalid geocoding result field");
}

struct AmiWeatherResult amiweather_openmeteo_parse_current(
    const void *json, size_t json_size, struct AmiWeatherCurrent *current)
{
    struct AmiAPIJsonDoc *doc;
    const struct AmiAPIJsonValue *root;
    const struct AmiAPIJsonValue *object;
    struct AmiWeatherCurrent parsed;
    enum AmiAPIJsonError json_error;
    size_t offset = 0;
    int rc;

    if (!json || !current)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "null parser argument");
    memset(&parsed, 0, sizeof(parsed));
    doc = AmiAPI_JSONParse(json, json_size, &json_error, &offset);
    if (!doc) {
        char detail[96];
        snprintf(detail, sizeof(detail), "JSON error %d at byte %lu",
                 (int)json_error, (unsigned long)offset);
        return result_make(AMIWEATHER_ERR_JSON, 0, detail);
    }
    root = AmiAPI_JSONRoot(doc);
    object = root && AmiAPI_JSONType(root) == AMIAPI_JSON_OBJECT
        ? AmiAPI_JSONObjectGet(doc, root, "current") : NULL;
    if (!object || AmiAPI_JSONType(object) != AMIAPI_JSON_OBJECT) {
        AmiAPI_JSONFree(doc);
        return result_make(AMIWEATHER_ERR_MISSING_FIELD, 0,
                           "missing current object");
    }
#define GET_NUMBER(field, member) \
    do { rc = json_number(doc, object, field, &parsed.member); \
         if (rc <= 0) goto field_error; } while (0)
    GET_NUMBER("temperature_2m", temperature_c);
    GET_NUMBER("apparent_temperature", apparent_temperature_c);
    GET_NUMBER("relative_humidity_2m", relative_humidity_percent);
    GET_NUMBER("wind_speed_10m", wind_speed_kmh);
    rc = json_long(doc, object, "weather_code", &parsed.weather_code);
    if (rc <= 0) goto field_error;
    rc = json_string(doc, object, "time", parsed.observation_time,
                     sizeof(parsed.observation_time));
    if (rc <= 0) goto field_error;
#undef GET_NUMBER
    if (parsed.relative_humidity_percent < 0.0 ||
        parsed.relative_humidity_percent > 100.0 ||
        parsed.wind_speed_kmh < 0.0 || parsed.weather_code < 0 ||
        parsed.weather_code > 99 || parsed.observation_time[0] == '\0') {
        AmiAPI_JSONFree(doc);
        return result_make(AMIWEATHER_ERR_INVALID_FIELD, 0,
                           "weather field outside expected range");
    }
    *current = parsed;
    AmiAPI_JSONFree(doc);
    return result_make(AMIWEATHER_OK, 0, NULL);

field_error:
    AmiAPI_JSONFree(doc);
    return result_make(rc == 0 ? AMIWEATHER_ERR_MISSING_FIELD
                               : AMIWEATHER_ERR_INVALID_FIELD,
                       0, rc == 0 ? "required current field is absent"
                                   : "required current field has wrong type");
}

struct AmiWeatherResult amiweather_openmeteo_parse_report(
    const void *json, size_t json_size, struct AmiWeatherReport *report)
{
    struct AmiAPIJsonDoc *doc;
    const struct AmiAPIJsonValue *root;
    const struct AmiAPIJsonValue *current_object;
    const struct AmiAPIJsonValue *daily;
    const struct AmiAPIJsonValue *times;
    const struct AmiAPIJsonValue *codes;
    const struct AmiAPIJsonValue *maximums;
    const struct AmiAPIJsonValue *minimums;
    const struct AmiAPIJsonValue *precipitation;
    const struct AmiAPIJsonValue *hourly;
    const struct AmiAPIJsonValue *hourly_times;
    const struct AmiAPIJsonValue *hourly_temperatures;
    const struct AmiAPIJsonValue *hourly_codes;
    const struct AmiAPIJsonValue *hourly_precipitation;
    static struct AmiWeatherReport parsed;
    enum AmiAPIJsonError json_error;
    size_t offset = 0;
    size_t count, i, hourly_count, hourly_start;
    int rc;
    char previous_hourly_time[AMIWEATHER_HOURLY_TIME_SIZE];

    if (!json || !report)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "null parser argument");
    memset(&parsed, 0, sizeof(parsed));
    doc = AmiAPI_JSONParse(json, json_size, &json_error, &offset);
    if (!doc) {
        char detail[96];
        snprintf(detail, sizeof(detail), "JSON error %d at byte %lu",
                 (int)json_error, (unsigned long)offset);
        return result_make(AMIWEATHER_ERR_JSON, 0, detail);
    }
    root = AmiAPI_JSONRoot(doc);
    if (!root || AmiAPI_JSONType(root) != AMIAPI_JSON_OBJECT)
        goto missing;
    current_object = AmiAPI_JSONObjectGet(doc, root, "current");
    daily = AmiAPI_JSONObjectGet(doc, root, "daily");
    if (!current_object || AmiAPI_JSONType(current_object) != AMIAPI_JSON_OBJECT ||
        !daily || AmiAPI_JSONType(daily) != AMIAPI_JSON_OBJECT)
        goto missing;
#define REPORT_NUMBER(field, member) \
    do { rc = json_number(doc, current_object, field, &parsed.current.member); \
         if (rc == 0) goto missing; \
         if (rc < 0) goto invalid; } while (0)
    REPORT_NUMBER("temperature_2m", temperature_c);
    REPORT_NUMBER("apparent_temperature", apparent_temperature_c);
    REPORT_NUMBER("relative_humidity_2m", relative_humidity_percent);
    REPORT_NUMBER("wind_speed_10m", wind_speed_kmh);
#undef REPORT_NUMBER
    rc = json_long(doc, current_object, "weather_code", &parsed.current.weather_code);
    if (rc == 0) goto missing;
    if (rc < 0) goto invalid;
    rc = json_string(doc, current_object, "time", parsed.current.observation_time,
                     sizeof(parsed.current.observation_time));
    if (rc == 0) goto missing;
    if (rc < 0) goto invalid;
    if (parsed.current.relative_humidity_percent < 0.0 ||
        parsed.current.relative_humidity_percent > 100.0 ||
        parsed.current.wind_speed_kmh < 0.0 || parsed.current.weather_code < 0 ||
        parsed.current.weather_code > 99 || !parsed.current.observation_time[0])
        goto invalid;
    {
        char compact_time[AMIWEATHER_COMPACT_TIME_SIZE];
        if (!amiweather_format_hourly_time(parsed.current.observation_time,
                                           compact_time, sizeof(compact_time)))
            goto invalid;
    }
    {
        double value;
#define OPTIONAL_CURRENT(field, member, minimum, maximum, flag) \
        do { rc = json_optional_number(doc, current_object, field, &value); \
             if (rc < 0 || (rc > 0 && (value < (minimum) || value > (maximum)))) goto invalid; \
             if (rc > 0) { parsed.current.member = value; parsed.current.flag = 1; } } while (0)
        OPTIONAL_CURRENT("wind_direction_10m", wind_direction_degrees, 0.0, 360.0, has_wind_direction);
        OPTIONAL_CURRENT("wind_gusts_10m", wind_gusts, 0.0, 1000.0, has_wind_gusts);
        OPTIONAL_CURRENT("surface_pressure", surface_pressure, 0.0, 2000.0, has_surface_pressure);
        OPTIONAL_CURRENT("cloud_cover", cloud_cover_percent, 0.0, 100.0, has_cloud_cover);
        OPTIONAL_CURRENT("visibility", visibility_m, 0.0, 1000000.0, has_visibility);
        OPTIONAL_CURRENT("precipitation", precipitation_mm, 0.0, 10000.0, has_precipitation);
#undef OPTIONAL_CURRENT
    }

    times = AmiAPI_JSONObjectGet(doc, daily, "time");
    codes = AmiAPI_JSONObjectGet(doc, daily, "weather_code");
    maximums = AmiAPI_JSONObjectGet(doc, daily, "temperature_2m_max");
    minimums = AmiAPI_JSONObjectGet(doc, daily, "temperature_2m_min");
    precipitation = AmiAPI_JSONObjectGet(doc, daily,
                                         "precipitation_probability_max");
    if (!times || !codes || !maximums || !minimums)
        goto missing;
    if (AmiAPI_JSONType(times) != AMIAPI_JSON_ARRAY ||
        AmiAPI_JSONType(codes) != AMIAPI_JSON_ARRAY ||
        AmiAPI_JSONType(maximums) != AMIAPI_JSON_ARRAY ||
        AmiAPI_JSONType(minimums) != AMIAPI_JSON_ARRAY ||
        (precipitation && AmiAPI_JSONType(precipitation) != AMIAPI_JSON_ARRAY))
        goto invalid;
    count = AmiAPI_JSONArraySize(times);
    if (count == 0)
        goto invalid;
    if (AmiAPI_JSONArraySize(codes) != count ||
        AmiAPI_JSONArraySize(maximums) != count ||
        AmiAPI_JSONArraySize(minimums) != count)
        goto invalid;
    if (count > AMIWEATHER_MAX_DAILY)
        count = AMIWEATHER_MAX_DAILY;
    for (i = 0; i < count; ++i) {
        struct AmiWeatherDaily *day = &parsed.forecast.days[i];
        char compact[AMIWEATHER_COMPACT_DATE_SIZE];
        const struct AmiAPIJsonValue *probability_value;
        long probability;
        if (!json_array_string(doc, times, i, day->date, sizeof(day->date)) ||
            !amiweather_format_forecast_date(day->date, compact, sizeof(compact)) ||
            !json_array_long(doc, codes, i, &day->weather_code) ||
            !json_array_number(doc, minimums, i, &day->temperature_min_c) ||
            !json_array_number(doc, maximums, i, &day->temperature_max_c) ||
            day->weather_code < 0 || day->weather_code > 99 ||
            day->temperature_min_c < -200.0 || day->temperature_min_c > 250.0 ||
            day->temperature_max_c < -200.0 || day->temperature_max_c > 250.0 ||
            day->temperature_min_c > day->temperature_max_c)
            goto invalid;
        if (!precipitation || i >= AmiAPI_JSONArraySize(precipitation))
            continue;
        probability_value = AmiAPI_JSONArrayGet(doc, precipitation, i);
        if (probability_value && AmiAPI_JSONType(probability_value) == AMIAPI_JSON_NULL)
            continue;
        if (!json_array_long(doc, precipitation, i, &probability) ||
            probability < 0 || probability > 100)
            goto invalid;
        day->precipitation_probability = (int)probability;
        day->has_precipitation_probability = 1;
    }
    parsed.forecast.count = count;

    hourly = AmiAPI_JSONObjectGet(doc, root, "hourly");
    if (!hourly || AmiAPI_JSONType(hourly) != AMIAPI_JSON_OBJECT)
        goto missing;
    hourly_times = AmiAPI_JSONObjectGet(doc, hourly, "time");
    hourly_temperatures = AmiAPI_JSONObjectGet(doc, hourly, "temperature_2m");
    hourly_codes = AmiAPI_JSONObjectGet(doc, hourly, "weather_code");
    hourly_precipitation = AmiAPI_JSONObjectGet(doc, hourly,
                                                "precipitation_probability");
    if (!hourly_times || !hourly_temperatures || !hourly_codes)
        goto missing;
    if (AmiAPI_JSONType(hourly_times) != AMIAPI_JSON_ARRAY ||
        AmiAPI_JSONType(hourly_temperatures) != AMIAPI_JSON_ARRAY ||
        AmiAPI_JSONType(hourly_codes) != AMIAPI_JSON_ARRAY ||
        (hourly_precipitation &&
         AmiAPI_JSONType(hourly_precipitation) != AMIAPI_JSON_ARRAY))
        goto invalid;
    hourly_count = AmiAPI_JSONArraySize(hourly_times);
    if (hourly_count == 0 || AmiAPI_JSONArraySize(hourly_temperatures) != hourly_count ||
        AmiAPI_JSONArraySize(hourly_codes) != hourly_count)
        goto invalid;
    hourly_start = hourly_count;
    previous_hourly_time[0] = '\0';
    for (i = 0; i < hourly_count; ++i) {
        char timestamp[AMIWEATHER_HOURLY_TIME_SIZE];
        char compact[AMIWEATHER_COMPACT_TIME_SIZE];
        double temperature;
        long weather_code;
        if (!json_array_string(doc, hourly_times, i, timestamp, sizeof(timestamp)) ||
            !amiweather_format_hourly_time(timestamp, compact, sizeof(compact)) ||
            !json_array_number(doc, hourly_temperatures, i, &temperature) ||
            !json_array_long(doc, hourly_codes, i, &weather_code) ||
            temperature < -200.0 || temperature > 250.0 ||
            weather_code < 0 || weather_code > 99)
            goto invalid;
        if (previous_hourly_time[0] && strcmp(timestamp, previous_hourly_time) <= 0)
            goto invalid;
        memcpy(previous_hourly_time, timestamp, sizeof(previous_hourly_time));
        if (hourly_start == hourly_count &&
            strncmp(timestamp, parsed.current.observation_time, 13) >= 0)
            hourly_start = i;
    }
    if (hourly_start == hourly_count)
        goto invalid;
    for (i = hourly_start; i < hourly_count &&
         parsed.hourly.count < AMIWEATHER_HOURLY_COUNT; ++i) {
        struct AmiWeatherHourly *entry =
            &parsed.hourly.entries[parsed.hourly.count];
        const struct AmiAPIJsonValue *probability_value;
        long probability;
        char compact[AMIWEATHER_COMPACT_TIME_SIZE];
        if (!json_array_string(doc, hourly_times, i, entry->time,
                               sizeof(entry->time)) ||
            !amiweather_format_hourly_time(entry->time, compact, sizeof(compact)) ||
            !json_array_number(doc, hourly_temperatures, i, &entry->temperature) ||
            !json_array_long(doc, hourly_codes, i, &entry->weather_code) ||
            entry->temperature < -200.0 || entry->temperature > 250.0 ||
            entry->weather_code < 0 || entry->weather_code > 99)
            goto invalid;
        if (hourly_precipitation &&
            i < AmiAPI_JSONArraySize(hourly_precipitation)) {
            probability_value = AmiAPI_JSONArrayGet(doc, hourly_precipitation, i);
            if (!probability_value ||
                AmiAPI_JSONType(probability_value) != AMIAPI_JSON_NULL) {
                if (!json_array_long(doc, hourly_precipitation, i, &probability) ||
                    probability < 0 || probability > 100)
                    goto invalid;
                entry->precipitation_probability = (int)probability;
                entry->has_precipitation_probability = 1;
            }
        }
        ++parsed.hourly.count;
    }
    if (parsed.hourly.count == 0)
        goto invalid;
    *report = parsed;
    AmiAPI_JSONFree(doc);
    return result_make(AMIWEATHER_OK, 0, NULL);
missing:
    AmiAPI_JSONFree(doc);
    return result_make(AMIWEATHER_ERR_MISSING_FIELD, 0,
                       "required current, daily, or hourly field is absent");
invalid:
    AmiAPI_JSONFree(doc);
    return result_make(AMIWEATHER_ERR_INVALID_FIELD, 0,
                       "invalid current, daily, or hourly weather field");
}

static struct AmiWeatherResult execute_query(struct AmiAPIQuery *query,
    struct AmiWeatherCurrent *current, struct AmiWeatherReport *report,
    struct AmiWeatherLocationList *locations)
{
    struct AmiAPIContext *context = NULL;
    struct AmiAPIRequest *request = NULL;
    struct AmiAPIResponse *response = NULL;
    struct AmiWeatherResult result;
    enum AmiAPIError api_error = AMIAPI_OK;
    int status = 0;

    context = AmiAPI_Create();
    if (!context) {
        result = result_make(AMIWEATHER_ERR_MEMORY, 0, "AmiAPI_Create failed");
        goto cleanup;
    }
    AmiAPI_SetMaxBody(context, OPENMETEO_MAX_BODY);
    AmiAPI_SetTimeout(context, OPENMETEO_TIMEOUT_SECONDS);
    request = AmiAPI_RequestCreate(context, "GET", AmiAPI_QueryURL(query));
    if (!request) {
        api_error = AmiAPI_LastError(context);
        goto api_failure;
    }
    response = AmiAPI_RequestExecute(request);
    if (!response) {
        api_error = AmiAPI_LastError(context);
        goto api_failure;
    }
    status = AmiAPI_Status(response);
    if (status < 200 || status > 299) {
        char detail[64];
        snprintf(detail, sizeof(detail), "Open-Meteo returned HTTP %d", status);
        result = result_make(AMIWEATHER_ERR_HTTP_STATUS, status, detail);
        goto cleanup;
    }
    if (report)
        result = amiweather_openmeteo_parse_report(
            AmiAPI_Body(response), AmiAPI_BodySize(response), report);
    else if (current)
        result = amiweather_openmeteo_parse_current(
            AmiAPI_Body(response), AmiAPI_BodySize(response), current);
    else
        result = amiweather_openmeteo_parse_locations(
            AmiAPI_Body(response), AmiAPI_BodySize(response), locations);
    result.http_status = status;
    goto cleanup;

api_failure:
    result = result_make(api_error == AMIAPI_ERR_NO_MEMORY
                             ? AMIWEATHER_ERR_MEMORY : AMIWEATHER_ERR_NETWORK,
                         0, AmiAPI_ErrorString(api_error));
cleanup:
    AmiAPI_FreeResponse(response);
    AmiAPI_RequestDestroy(request);
    AmiAPI_Destroy(context);
    return result;
}

static void format_coordinate(char *buffer, size_t size, double value)
{
    long millionths = value >= 0.0
        ? (long)(value * 1000000.0 + 0.5)
        : (long)(value * 1000000.0 - 0.5);
    unsigned long magnitude = millionths < 0
        ? (unsigned long)(-(millionths + 1L)) + 1UL
        : (unsigned long)millionths;
    snprintf(buffer, size, "%s%lu.%06lu", millionths < 0 ? "-" : "",
             magnitude / 1000000UL, magnitude % 1000000UL);
}

struct AmiWeatherResult amiweather_openmeteo_fetch_current(
    const struct AmiWeatherLocation *location, struct AmiWeatherCurrent *current)
{
    struct AmiAPIQuery *query;
    struct AmiWeatherResult result;
    enum AmiAPIError error;
    char latitude[24];
    char longitude[24];
    const char *timezone;

    if (!location || !current || !location->name[0] ||
        location->latitude < -90.0 || location->latitude > 90.0 ||
        location->longitude < -180.0 || location->longitude > 180.0)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "invalid location");
    format_coordinate(latitude, sizeof(latitude), location->latitude);
    format_coordinate(longitude, sizeof(longitude), location->longitude);
    timezone = location->timezone[0] ? location->timezone : "auto";
    query = AmiAPI_QueryCreate(AMIWEATHER_OPENMETEO_BASE_URL);
    if (!query)
        return result_make(AMIWEATHER_ERR_MEMORY, 0, "query allocation failed");
#define ADD(name, value) do { error = AmiAPI_QueryAdd(query, name, value); \
    if (error != AMIAPI_OK) goto add_failed; } while (0)
    ADD("latitude", latitude);
    ADD("longitude", longitude);
    ADD("current", "temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,wind_gusts_10m,surface_pressure,cloud_cover,visibility,precipitation");
    ADD("temperature_unit", "celsius");
    ADD("wind_speed_unit", "kmh");
    ADD("timezone", timezone);
#undef ADD
    result = execute_query(query, current, NULL, NULL);
    AmiAPI_QueryDestroy(query);
    return result;
add_failed:
    AmiAPI_QueryDestroy(query);
    return result_make(error == AMIAPI_ERR_NO_MEMORY ? AMIWEATHER_ERR_MEMORY
                                                     : AMIWEATHER_ERR_NETWORK,
                       0, AmiAPI_ErrorString(error));
}

static struct AmiAPIQuery *report_query(
    const struct AmiWeatherLocation *location,
    enum AmiWeatherTemperatureUnit temperature_unit,
    enum AmiWeatherWindUnit wind_unit,
    enum AmiAPIError *query_error)
{
    struct AmiAPIQuery *query;
    enum AmiAPIError error;
    char latitude[24];
    char longitude[24];
    const char *timezone;

    *query_error = AMIAPI_OK;
    if (!location || !location->name[0] ||
        (temperature_unit != AMIWEATHER_TEMPERATURE_CELSIUS &&
         temperature_unit != AMIWEATHER_TEMPERATURE_FAHRENHEIT) ||
        (wind_unit != AMIWEATHER_WIND_KMH && wind_unit != AMIWEATHER_WIND_MPH) ||
        location->latitude < -90.0 || location->latitude > 90.0 ||
        location->longitude < -180.0 || location->longitude > 180.0)
        return NULL;
    format_coordinate(latitude, sizeof(latitude), location->latitude);
    format_coordinate(longitude, sizeof(longitude), location->longitude);
    timezone = location->timezone[0] ? location->timezone : "auto";
    query = AmiAPI_QueryCreate(AMIWEATHER_OPENMETEO_BASE_URL);
    if (!query) { *query_error = AMIAPI_ERR_NO_MEMORY; return NULL; }
#define ADD_REPORT(name, value) do { error = AmiAPI_QueryAdd(query, name, value); \
    if (error != AMIAPI_OK) goto report_add_failed; } while (0)
    ADD_REPORT("latitude", latitude);
    ADD_REPORT("longitude", longitude);
    ADD_REPORT("current", "temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,wind_gusts_10m,surface_pressure,cloud_cover,visibility,precipitation");
    ADD_REPORT("daily", "weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max");
    ADD_REPORT("hourly", "temperature_2m,weather_code,precipitation_probability");
    ADD_REPORT("temperature_unit", amiweather_temperature_unit_query(temperature_unit));
    ADD_REPORT("wind_speed_unit", amiweather_wind_unit_query(wind_unit));
    ADD_REPORT("timezone", timezone);
    ADD_REPORT("forecast_days", "14");
#undef ADD_REPORT
    return query;
report_add_failed:
    *query_error = error;
    AmiAPI_QueryDestroy(query);
    return NULL;
}

struct AmiWeatherResult amiweather_openmeteo_report_url(
    const struct AmiWeatherLocation *location,
    enum AmiWeatherTemperatureUnit temperature_unit,
    enum AmiWeatherWindUnit wind_unit, char *buffer, size_t size)
{
    struct AmiAPIQuery *query;
    enum AmiAPIError error;
    const char *url;
    if (!buffer || size == 0)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "invalid URL buffer");
    query = report_query(location, temperature_unit, wind_unit, &error);
    if (!query)
        return result_make(error == AMIAPI_ERR_NO_MEMORY ? AMIWEATHER_ERR_MEMORY
                                                         : AMIWEATHER_ERR_ARGUMENT,
                           0, "query construction failed");
    url = AmiAPI_QueryURL(query);
    if (!url || strlen(url) >= size) {
        AmiAPI_QueryDestroy(query);
        return result_make(AMIWEATHER_ERR_MEMORY, 0, "URL buffer too small");
    }
    memcpy(buffer, url, strlen(url) + 1);
    AmiAPI_QueryDestroy(query);
    return result_make(AMIWEATHER_OK, 0, NULL);
}

struct AmiWeatherResult amiweather_openmeteo_fetch_report_units(
    const struct AmiWeatherLocation *location,
    enum AmiWeatherTemperatureUnit temperature_unit,
    enum AmiWeatherWindUnit wind_unit, struct AmiWeatherReport *report)
{
    struct AmiAPIQuery *query;
    struct AmiWeatherResult result;
    enum AmiAPIError error;
    if (!report)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "invalid report");
    query = report_query(location, temperature_unit, wind_unit, &error);
    if (!query)
        return result_make(error == AMIAPI_ERR_NO_MEMORY ? AMIWEATHER_ERR_MEMORY
                                                         : AMIWEATHER_ERR_ARGUMENT,
                           0, "query construction failed");
    result = execute_query(query, NULL, report, NULL);
    AmiAPI_QueryDestroy(query);
    if (result.error == AMIWEATHER_OK) {
        report->temperature_unit = temperature_unit;
        report->wind_unit = wind_unit;
    }
    return result;
}

struct AmiWeatherResult amiweather_openmeteo_fetch_report(
    const struct AmiWeatherLocation *location, struct AmiWeatherReport *report)
{
    return amiweather_openmeteo_fetch_report_units(
        location, AMIWEATHER_TEMPERATURE_CELSIUS, AMIWEATHER_WIND_KMH, report);
}

struct AmiWeatherResult amiweather_openmeteo_fetch_reference(
    struct AmiWeatherCurrent *current)
{
    struct AmiWeatherLocation location;
    amiweather_location_default(&location);
    return amiweather_openmeteo_fetch_current(&location, current);
}

struct AmiWeatherResult amiweather_geocode_search(
    const char *search, struct AmiWeatherLocationList *locations)
{
    struct AmiAPIQuery *query;
    struct AmiWeatherResult result;
    enum AmiAPIError error;
    const char *p;

    if (!search || !locations)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "null search argument");
    for (p = search; *p == ' ' || *p == '\t'; ++p) {}
    if (!*p)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "empty location search");
    memset(locations, 0, sizeof(*locations));
    query = AmiAPI_QueryCreate(AMIWEATHER_OPENMETEO_GEOCODING_URL);
    if (!query)
        return result_make(AMIWEATHER_ERR_MEMORY, 0, "query allocation failed");
    error = AmiAPI_QueryAdd(query, "name", p);
    if (error == AMIAPI_OK)
        error = AmiAPI_QueryAdd(query, "count", OPENMETEO_GEOCODING_LIMIT);
    if (error == AMIAPI_OK)
        error = AmiAPI_QueryAdd(query, "language", "en");
    if (error != AMIAPI_OK) {
        AmiAPI_QueryDestroy(query);
        return result_make(error == AMIAPI_ERR_NO_MEMORY ? AMIWEATHER_ERR_MEMORY
                                                         : AMIWEATHER_ERR_NETWORK,
                           0, AmiAPI_ErrorString(error));
    }
    result = execute_query(query, NULL, NULL, locations);
    AmiAPI_QueryDestroy(query);
    return result;
}
