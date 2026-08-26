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

struct AmiWeatherResult amiweather_openmeteo_fetch_reference(
    struct AmiWeatherCurrent *current)
{
    struct AmiAPIContext *context = NULL;
    struct AmiAPIQuery *query = NULL;
    struct AmiAPIRequest *request = NULL;
    struct AmiAPIResponse *response = NULL;
    struct AmiWeatherResult result;
    enum AmiAPIError api_error = AMIAPI_OK;
    int status = 0;

    if (!current)
        return result_make(AMIWEATHER_ERR_ARGUMENT, 0, "null weather output");
    context = AmiAPI_Create();
    if (!context) {
        result = result_make(AMIWEATHER_ERR_MEMORY, 0, "AmiAPI_Create failed");
        goto cleanup;
    }
    AmiAPI_SetMaxBody(context, OPENMETEO_MAX_BODY);
    AmiAPI_SetTimeout(context, OPENMETEO_TIMEOUT_SECONDS);
    query = AmiAPI_QueryCreate(AMIWEATHER_OPENMETEO_BASE_URL);
    if (!query) {
        result = result_make(AMIWEATHER_ERR_MEMORY, 0, "query allocation failed");
        goto cleanup;
    }
#define ADD_QUERY(name, value) \
    do { api_error = AmiAPI_QueryAdd(query, name, value); \
         if (api_error != AMIAPI_OK) goto api_failure; } while (0)
    ADD_QUERY("latitude", "59.9139");
    ADD_QUERY("longitude", "10.7522");
    ADD_QUERY("current", "temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m");
    ADD_QUERY("temperature_unit", "celsius");
    ADD_QUERY("wind_speed_unit", "kmh");
    ADD_QUERY("timezone", "UTC");
#undef ADD_QUERY
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
    result = amiweather_openmeteo_parse_current(
        AmiAPI_Body(response), AmiAPI_BodySize(response), current);
    result.http_status = status;
    goto cleanup;

api_failure:
    result = result_make(api_error == AMIAPI_ERR_NO_MEMORY
                             ? AMIWEATHER_ERR_MEMORY : AMIWEATHER_ERR_NETWORK,
                         0, AmiAPI_ErrorString(api_error));
cleanup:
    AmiAPI_FreeResponse(response);
    AmiAPI_RequestDestroy(request);
    AmiAPI_QueryDestroy(query);
    AmiAPI_Destroy(context);
    return result;
}
