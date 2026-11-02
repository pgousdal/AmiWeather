#ifdef AMIWEATHER_AMIGA
#include <dos/dos.h>
#include <proto/dos.h>
#endif
#include <amiweather/preferences.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AMIWEATHER_AMIGA
#include <exec/types.h>
#include <proto/dos.h>
#endif

#define PREFERENCES_FILE_SIZE 8192

static int path_remove(const char *path)
{
#ifdef AMIWEATHER_AMIGA
    return DeleteFile((CONST_STRPTR)path) ? 0 : -1;
#else
    return remove(path);
#endif
}

static int path_rename(const char *old_path, const char *new_path)
{
#ifdef AMIWEATHER_AMIGA
    return Rename((CONST_STRPTR)old_path, (CONST_STRPTR)new_path) ? 0 : -1;
#else
    return rename(old_path, new_path);
#endif
}

void amiweather_preferences_default(struct AmiWeatherPreferences *preferences)
{
    if (!preferences)
        return;
    memset(preferences, 0, sizeof(*preferences));
    preferences->format_version = AMIWEATHER_PREFERENCES_VERSION;
    preferences->temperature_unit = AMIWEATHER_TEMPERATURE_CELSIUS;
    preferences->wind_unit = AMIWEATHER_WIND_KMH;
}

const char *amiweather_preferences_error_string(enum AmiWeatherPreferencesError error)
{
    switch (error) {
    case AMIWEATHER_PREFS_OK: return "success";
    case AMIWEATHER_PREFS_NOT_FOUND: return "preferences not found";
    case AMIWEATHER_PREFS_INVALID: return "invalid preferences";
    case AMIWEATHER_PREFS_UNSUPPORTED_VERSION: return "unsupported preferences version";
    case AMIWEATHER_PREFS_IO_ERROR: return "preferences I/O failed";
    case AMIWEATHER_PREFS_FULL: return "saved-location list is full";
    case AMIWEATHER_PREFS_DUPLICATE: return "location is already saved";
    default: return "unknown preferences error";
    }
}

const char *amiweather_temperature_unit_query(enum AmiWeatherTemperatureUnit unit)
{
    return unit == AMIWEATHER_TEMPERATURE_FAHRENHEIT ? "fahrenheit" : "celsius";
}

const char *amiweather_temperature_unit_label(enum AmiWeatherTemperatureUnit unit)
{
    return unit == AMIWEATHER_TEMPERATURE_FAHRENHEIT ? "F" : "C";
}

const char *amiweather_wind_unit_query(enum AmiWeatherWindUnit unit)
{
    return unit == AMIWEATHER_WIND_MPH ? "mph" : "kmh";
}

const char *amiweather_wind_unit_label(enum AmiWeatherWindUnit unit)
{
    return unit == AMIWEATHER_WIND_MPH ? "mph" : "km/h";
}

static int safe_field(const char *field)
{
    return field && !strchr(field, '|') && !strchr(field, '\n') &&
           !strchr(field, '\r');
}

int amiweather_location_equal(const struct AmiWeatherLocation *left,
                              const struct AmiWeatherLocation *right)
{
    long left_latitude, right_latitude, left_longitude, right_longitude;
    if (!left || !right)
        return 0;
    left_latitude = (long)(left->latitude * 1000000.0 +
                           (left->latitude >= 0.0 ? 0.5 : -0.5));
    right_latitude = (long)(right->latitude * 1000000.0 +
                            (right->latitude >= 0.0 ? 0.5 : -0.5));
    left_longitude = (long)(left->longitude * 1000000.0 +
                            (left->longitude >= 0.0 ? 0.5 : -0.5));
    right_longitude = (long)(right->longitude * 1000000.0 +
                             (right->longitude >= 0.0 ? 0.5 : -0.5));
    return left_latitude == right_latitude &&
           left_longitude == right_longitude &&
           strcmp(left->name, right->name) == 0 &&
           strcmp(left->country_code, right->country_code) == 0 &&
           strcmp(left->timezone, right->timezone) == 0;
}

enum AmiWeatherPreferencesError amiweather_preferences_add_location(
    struct AmiWeatherPreferences *preferences,
    const struct AmiWeatherLocation *location)
{
    size_t i;
    if (!preferences || !location || !location->name[0] ||
        location->latitude < -90.0 || location->latitude > 90.0 ||
        location->longitude < -180.0 || location->longitude > 180.0 ||
        !safe_field(location->name) || !safe_field(location->admin1) ||
        !safe_field(location->country) || !safe_field(location->country_code) ||
        !safe_field(location->timezone))
        return AMIWEATHER_PREFS_INVALID;
    for (i = 0; i < preferences->saved_location_count; ++i)
        if (amiweather_location_equal(&preferences->saved_locations[i], location))
            return AMIWEATHER_PREFS_DUPLICATE;
    if (preferences->saved_location_count >= AMIWEATHER_MAX_SAVED_LOCATIONS)
        return AMIWEATHER_PREFS_FULL;
    preferences->saved_locations[preferences->saved_location_count++] = *location;
    return AMIWEATHER_PREFS_OK;
}

enum AmiWeatherPreferencesError amiweather_preferences_remove_location(
    struct AmiWeatherPreferences *preferences, size_t index)
{
    size_t i;
    if (!preferences || index >= preferences->saved_location_count)
        return AMIWEATHER_PREFS_INVALID;
    for (i = index + 1; i < preferences->saved_location_count; ++i)
        preferences->saved_locations[i - 1] = preferences->saved_locations[i];
    --preferences->saved_location_count;
    memset(&preferences->saved_locations[preferences->saved_location_count], 0,
           sizeof(preferences->saved_locations[0]));
    if (preferences->saved_location_count == 0) {
        preferences->has_default_location = 0;
        preferences->default_location_index = 0;
    } else if (preferences->has_default_location) {
        if (index == preferences->default_location_index)
            preferences->default_location_index = 0;
        else if (index < preferences->default_location_index)
            --preferences->default_location_index;
    }
    return AMIWEATHER_PREFS_OK;
}

int amiweather_preferences_set_default(struct AmiWeatherPreferences *preferences,
                                       size_t index)
{
    if (!preferences || index >= preferences->saved_location_count)
        return 0;
    preferences->default_location_index = index;
    preferences->has_default_location = 1;
    return 1;
}

static int parse_unsigned(const char *text, unsigned long *value)
{
    char *end;
    errno = 0;
    *value = strtoul(text, &end, 10);
    return errno == 0 && *text && *end == '\0';
}

static int copy_part(char *destination, size_t size, const char *part)
{
    size_t length = strlen(part);
    if (length >= size)
        return 0;
    memcpy(destination, part, length + 1);
    return 1;
}

static int parse_location(char *value, struct AmiWeatherLocation *location)
{
    char *parts[7];
    char *cursor = value;
    char *separator;
    char *end;
    size_t i;
    memset(location, 0, sizeof(*location));
    for (i = 0; i < 7; ++i) {
        parts[i] = cursor;
        separator = strchr(cursor, '|');
        if (i < 6) {
            if (!separator) return 0;
            *separator = '\0';
            cursor = separator + 1;
        } else if (separator) return 0;
    }
    if (!copy_part(location->name, sizeof(location->name), parts[0]) ||
        !copy_part(location->admin1, sizeof(location->admin1), parts[1]) ||
        !copy_part(location->country, sizeof(location->country), parts[2]) ||
        !copy_part(location->country_code, sizeof(location->country_code), parts[3]) ||
        !copy_part(location->timezone, sizeof(location->timezone), parts[6]) ||
        !location->name[0] || !parts[4][0] || !parts[5][0])
        return 0;
    errno = 0;
    location->latitude = strtod(parts[4], &end);
    if (errno || *end || location->latitude != location->latitude ||
        location->latitude < -90.0 || location->latitude > 90.0)
        return 0;
    errno = 0;
    location->longitude = strtod(parts[5], &end);
    return !errno && !*end && location->longitude == location->longitude &&
           location->longitude >= -180.0 &&
           location->longitude <= 180.0;
}

enum AmiWeatherPreferencesError amiweather_preferences_parse(
    const char *data, size_t size, struct AmiWeatherPreferences *preferences)
{
    struct AmiWeatherPreferences parsed;
    size_t offset = 0;
    int saw_version = 0;
    int saw_default = 0;
    unsigned long default_index = 0;
    if (!data || !preferences)
        return AMIWEATHER_PREFS_INVALID;
    if (size == 0 || data[size - 1] != '\n')
        return AMIWEATHER_PREFS_INVALID;
    amiweather_preferences_default(&parsed);
    while (offset < size) {
        char line[AMIWEATHER_PREFERENCES_LINE_SIZE];
        size_t start = offset, length;
        char *equals, *key, *value;
        unsigned long number;
        while (offset < size && data[offset] != '\n') ++offset;
        length = offset - start;
        if (offset < size) ++offset;
        if (length && data[start + length - 1] == '\r') --length;
        if (length >= sizeof(line)) return AMIWEATHER_PREFS_INVALID;
        memcpy(line, data + start, length); line[length] = '\0';
        if (!line[0] || line[0] == '#') continue;
        equals = strchr(line, '=');
        if (!equals) return AMIWEATHER_PREFS_INVALID;
        *equals = '\0'; key = line; value = equals + 1;
        if (!strcmp(key, "version")) {
            if (saw_version || !parse_unsigned(value, &number))
                return AMIWEATHER_PREFS_INVALID;
            if (number != AMIWEATHER_PREFERENCES_VERSION)
                return AMIWEATHER_PREFS_UNSUPPORTED_VERSION;
            saw_version = 1;
        } else if (!strcmp(key, "temperature")) {
            if (!strcmp(value, "celsius")) parsed.temperature_unit = AMIWEATHER_TEMPERATURE_CELSIUS;
            else if (!strcmp(value, "fahrenheit")) parsed.temperature_unit = AMIWEATHER_TEMPERATURE_FAHRENHEIT;
            else return AMIWEATHER_PREFS_INVALID;
        } else if (!strcmp(key, "wind")) {
            if (!strcmp(value, "kmh")) parsed.wind_unit = AMIWEATHER_WIND_KMH;
            else if (!strcmp(value, "mph")) parsed.wind_unit = AMIWEATHER_WIND_MPH;
            else return AMIWEATHER_PREFS_INVALID;
        } else if (!strcmp(key, "default")) {
            if (!parse_unsigned(value, &default_index)) return AMIWEATHER_PREFS_INVALID;
            saw_default = 1;
        } else if (!strcmp(key, "location")) {
            struct AmiWeatherLocation location;
            enum AmiWeatherPreferencesError add_result;
            if (!parse_location(value, &location)) return AMIWEATHER_PREFS_INVALID;
            add_result = amiweather_preferences_add_location(&parsed, &location);
            if (add_result != AMIWEATHER_PREFS_OK) return add_result;
        }
    }
    if (!saw_version) return AMIWEATHER_PREFS_INVALID;
    if (saw_default) {
        if (default_index >= parsed.saved_location_count) return AMIWEATHER_PREFS_INVALID;
        parsed.default_location_index = (size_t)default_index;
        parsed.has_default_location = 1;
    }
    *preferences = parsed;
    return AMIWEATHER_PREFS_OK;
}

static int append_text(char *buffer, size_t size, size_t *used, const char *text)
{
    size_t length = strlen(text);
    if (*used + length >= size) return 0;
    memcpy(buffer + *used, text, length); *used += length; buffer[*used] = '\0';
    return 1;
}

static void coordinate_text(char *buffer, size_t size, double value)
{
    long millionths = (long)(value * 1000000.0 + (value >= 0.0 ? 0.5 : -0.5));
    unsigned long magnitude = millionths < 0
        ? (unsigned long)(-(millionths + 1L)) + 1UL : (unsigned long)millionths;
    snprintf(buffer, size, "%s%lu.%06lu", millionths < 0 ? "-" : "",
             magnitude / 1000000UL, magnitude % 1000000UL);
}

enum AmiWeatherPreferencesError amiweather_preferences_serialize(
    const struct AmiWeatherPreferences *preferences,
    char *buffer, size_t size, size_t *written)
{
    char line[AMIWEATHER_PREFERENCES_LINE_SIZE];
    size_t used = 0, i;
    if (!preferences || !buffer || !size ||
        preferences->format_version != AMIWEATHER_PREFERENCES_VERSION ||
        preferences->saved_location_count > AMIWEATHER_MAX_SAVED_LOCATIONS ||
        (preferences->temperature_unit != AMIWEATHER_TEMPERATURE_CELSIUS &&
         preferences->temperature_unit != AMIWEATHER_TEMPERATURE_FAHRENHEIT) ||
        (preferences->wind_unit != AMIWEATHER_WIND_KMH &&
         preferences->wind_unit != AMIWEATHER_WIND_MPH) ||
        (preferences->has_default_location &&
         preferences->default_location_index >= preferences->saved_location_count))
        return AMIWEATHER_PREFS_INVALID;
    buffer[0] = '\0';
    snprintf(line, sizeof(line), "version=%u\ntemperature=%s\nwind=%s\n",
             AMIWEATHER_PREFERENCES_VERSION,
             amiweather_temperature_unit_query(preferences->temperature_unit),
             amiweather_wind_unit_query(preferences->wind_unit));
    if (!append_text(buffer, size, &used, line)) return AMIWEATHER_PREFS_IO_ERROR;
    if (preferences->has_default_location) {
        snprintf(line, sizeof(line), "default=%lu\n",
                 (unsigned long)preferences->default_location_index);
        if (!append_text(buffer, size, &used, line)) return AMIWEATHER_PREFS_IO_ERROR;
    }
    for (i = 0; i < preferences->saved_location_count; ++i) {
        const struct AmiWeatherLocation *location = &preferences->saved_locations[i];
        char latitude[24], longitude[24];
        if (!safe_field(location->name) || !safe_field(location->admin1) ||
            !safe_field(location->country) || !safe_field(location->country_code) ||
            !safe_field(location->timezone)) return AMIWEATHER_PREFS_INVALID;
        coordinate_text(latitude, sizeof(latitude), location->latitude);
        coordinate_text(longitude, sizeof(longitude), location->longitude);
        snprintf(line, sizeof(line), "location=%s|%s|%s|%s|%s|%s|%s\n",
                 location->name, location->admin1, location->country,
                 location->country_code, latitude, longitude, location->timezone);
        if (strlen(line) >= sizeof(line) - 1 ||
            !append_text(buffer, size, &used, line)) return AMIWEATHER_PREFS_IO_ERROR;
    }
    if (written) *written = used;
    return AMIWEATHER_PREFS_OK;
}

/* A missing file (or drawer) is not an error: it is the first run.  The
 * Amiga C library does not always set errno to ENOENT, so ask DOS. */
static int file_missing(const char *path)
{
#ifdef AMIWEATHER_AMIGA
    BPTR lock;
    LONG error;
    if (errno == ENOENT) return 1;
    lock = Lock((CONST_STRPTR)path, ACCESS_READ);
    if (lock) { UnLock(lock); return 0; }
    error = IoErr();
    return error == ERROR_OBJECT_NOT_FOUND || error == ERROR_DIR_NOT_FOUND;
#else
    (void)path;
    return errno == ENOENT;
#endif
}

enum AmiWeatherPreferencesError amiweather_preferences_load_file(
    const char *path, struct AmiWeatherPreferences *preferences)
{
    FILE *file;
    char *data;
    size_t length;
    enum AmiWeatherPreferencesError result;
    if (!path || !preferences) return AMIWEATHER_PREFS_INVALID;
    file = fopen(path, "rb");
    if (!file) return file_missing(path) ? AMIWEATHER_PREFS_NOT_FOUND : AMIWEATHER_PREFS_IO_ERROR;
    data = (char *)malloc(PREFERENCES_FILE_SIZE + 1);
    if (!data) { fclose(file); return AMIWEATHER_PREFS_IO_ERROR; }
    length = fread(data, 1, PREFERENCES_FILE_SIZE + 1, file);
    if (ferror(file) || length > PREFERENCES_FILE_SIZE || fclose(file) != 0) {
        free(data); return AMIWEATHER_PREFS_IO_ERROR;
    }
    result = amiweather_preferences_parse(data, length, preferences);
    free(data);
    return result;
}

enum AmiWeatherPreferencesError amiweather_preferences_save_file(
    const char *path, struct AmiWeatherPreferences *preferences)
{
    char *data;
    char temporary[256], backup[256];
    size_t length;
    FILE *file;
    FILE *existing;
    int write_failed = 0;
    enum AmiWeatherPreferencesError result;
    if (!path || strlen(path) > 240) return AMIWEATHER_PREFS_INVALID;
    data = (char *)malloc(PREFERENCES_FILE_SIZE);
    if (!data) return AMIWEATHER_PREFS_IO_ERROR;
    result = amiweather_preferences_serialize(preferences, data,
                                              PREFERENCES_FILE_SIZE, &length);
    if (result != AMIWEATHER_PREFS_OK) { free(data); return result; }
    snprintf(temporary, sizeof(temporary), "%s.new", path);
    snprintf(backup, sizeof(backup), "%s.old", path);
    file = fopen(temporary, "wb");
    if (!file) { free(data); return AMIWEATHER_PREFS_IO_ERROR; }
    if (fwrite(data, 1, length, file) != length || fflush(file) != 0)
        write_failed = 1;
    if (fclose(file) != 0)
        write_failed = 1;
    if (write_failed) { path_remove(temporary); free(data); return AMIWEATHER_PREFS_IO_ERROR; }
    free(data);
    path_remove(backup);
    existing = fopen(path, "rb");
    if (existing) {
        if (fclose(existing) != 0 || path_rename(path, backup) != 0) {
            path_remove(temporary); return AMIWEATHER_PREFS_IO_ERROR;
        }
    }
    if (path_rename(temporary, path) != 0) {
        path_rename(backup, path); path_remove(temporary); return AMIWEATHER_PREFS_IO_ERROR;
    }
    path_remove(backup);
    return AMIWEATHER_PREFS_OK;
}

enum AmiWeatherPreferencesError amiweather_preferences_load_resolved(
    const char *env_path, const char *envarc_path,
    struct AmiWeatherPreferences *preferences)
{
    enum AmiWeatherPreferencesError env_result, archive_result;
    if (!env_path || !envarc_path || !preferences)
        return AMIWEATHER_PREFS_INVALID;
    env_result = amiweather_preferences_load_file(env_path, preferences);
    if (env_result == AMIWEATHER_PREFS_OK)
        return AMIWEATHER_PREFS_OK;
    archive_result = amiweather_preferences_load_file(envarc_path, preferences);
    if (archive_result == AMIWEATHER_PREFS_OK)
        return AMIWEATHER_PREFS_OK;
    if (env_result == AMIWEATHER_PREFS_NOT_FOUND &&
        archive_result == AMIWEATHER_PREFS_NOT_FOUND)
        return AMIWEATHER_PREFS_NOT_FOUND;
    return archive_result;
}

/* ENV: is a RAM copy.  On OS 3.2 the ENV handler mirrors ENVARC:, and the
 * temporary + rename dance of save_file fails there, so on Amiga the ENV:
 * copy is written in place.  ENVARC: keeps the safe save. */
static enum AmiWeatherPreferencesError save_env_copy(
    const char *path, struct AmiWeatherPreferences *preferences)
{
#ifdef AMIWEATHER_AMIGA
    char *data;
    size_t length;
    FILE *file;
    int failed = 0;
    enum AmiWeatherPreferencesError result;
    data = (char *)malloc(PREFERENCES_FILE_SIZE);
    if (!data) return AMIWEATHER_PREFS_IO_ERROR;
    result = amiweather_preferences_serialize(preferences, data,
                                              PREFERENCES_FILE_SIZE, &length);
    if (result != AMIWEATHER_PREFS_OK) { free(data); return result; }
    file = fopen(path, "wb");
    if (!file) { free(data); return AMIWEATHER_PREFS_IO_ERROR; }
    if (fwrite(data, 1, length, file) != length) failed = 1;
    if (fclose(file) != 0) failed = 1;
    free(data);
    return failed ? AMIWEATHER_PREFS_IO_ERROR : AMIWEATHER_PREFS_OK;
#else
    return amiweather_preferences_save_file(path, preferences);
#endif
}

enum AmiWeatherPreferencesError amiweather_preferences_save_persistent(
    const char *envarc_path, const char *env_path,
    struct AmiWeatherPreferences *preferences)
{
    enum AmiWeatherPreferencesError result;
    enum AmiWeatherPreferencesError previous_result;
    struct AmiWeatherPreferences previous;
    if (!envarc_path || !env_path || !preferences)
        return AMIWEATHER_PREFS_INVALID;
    previous_result = amiweather_preferences_load_file(envarc_path, &previous);
    result = amiweather_preferences_save_file(envarc_path, preferences);
    if (result != AMIWEATHER_PREFS_OK)
        return result;
    result = save_env_copy(env_path, preferences);
    if (result != AMIWEATHER_PREFS_OK) {
        if (previous_result == AMIWEATHER_PREFS_OK)
            (void)amiweather_preferences_save_file(envarc_path, &previous);
        else if (previous_result == AMIWEATHER_PREFS_NOT_FOUND)
            (void)path_remove(envarc_path);
    }
    return result;
}

enum AmiWeatherPreferencesError amiweather_preferences_set_default_location(
    struct AmiWeatherPreferences *preferences,
    const struct AmiWeatherLocation *location)
{
    size_t i;
    enum AmiWeatherPreferencesError result;
    if (!preferences || !location || !location->name[0])
        return AMIWEATHER_PREFS_INVALID;
    for (i = 0; i < preferences->saved_location_count; ++i) {
        if (amiweather_location_equal(&preferences->saved_locations[i], location)) {
            amiweather_preferences_set_default(preferences, i);
            return AMIWEATHER_PREFS_OK;
        }
    }
    result = amiweather_preferences_add_location(preferences, location);
    if (result != AMIWEATHER_PREFS_OK)
        return result;
    amiweather_preferences_set_default(preferences,
        preferences->saved_location_count - 1);
    return AMIWEATHER_PREFS_OK;
}
