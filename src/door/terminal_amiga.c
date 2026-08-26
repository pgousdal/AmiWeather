#include <amiweather/door.h>
#include <amiweather/openmeteo.h>
#include <amiweather/preferences.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#define AMIWEATHER_DOOR_INPUT_SIZE 64
struct AmiWeatherDoorState {
    struct AmiWeatherPreferences preferences;
    struct AmiWeatherLocation location;
    struct AmiWeatherReport report;
    int has_report;
    int has_location;
};

static struct AmiWeatherDoorState door_state;

static int amiga_write(void *context, const char *text, size_t length)
{
    (void)context;
    return (int)Write(Output(), (APTR)text, (LONG)length);
}

static int amiga_read_key(void *context, int *key)
{
    char c;
    (void)context;
    if (!key || Read(Input(), &c, 1) != 1) return 0;
    *key = (unsigned char)c;
    return 1;
}

static int amiga_connected(void *context)
{
    (void)context;
    return 1;
}

static int read_line(const struct AmiWeatherDoorTerminal *terminal,
                     char *buffer, size_t size)
{
    size_t length = 0;
    int started = 0;
    int key;
    if (!buffer || size < 2) return 0;
    while (terminal->read_key(terminal->context, &key)) {
        if (!started && (key == '\n' || key == '\r')) continue;
        if (key == '\n' || key == '\r') {
            buffer[length] = '\0';
            return 1;
        }
        started = 1;
        if (key == 8 || key == 127) {
            if (length) --length;
            continue;
        }
        if (length + 1 < size) buffer[length++] = (char)key;
    }
    return 0;
}

static int read_command(const struct AmiWeatherDoorTerminal *terminal, int *key)
{
    int value;
    if (!key) return 0;
    while (terminal->read_key(terminal->context, &value)) {
        if (value == '\n' || value == '\r') continue;
        *key = value;
        return 1;
    }
    return 0;
}

static void show_error(const struct AmiWeatherDoorTerminal *terminal,
                       const char *operation, const struct AmiWeatherResult *result)
{
    char line[256];
    if (result->diagnostic[0])
        snprintf(line, sizeof(line), "\n%s failed: %s (%s)\n", operation,
                 amiweather_error_string(result->error), result->diagnostic);
    else
        snprintf(line, sizeof(line), "\n%s failed: %s\n", operation,
                 amiweather_error_string(result->error));
    amiweather_door_write(terminal, line);
}

static int fetch_location(const struct AmiWeatherLocation *location,
                          struct AmiWeatherReport *report,
                          struct AmiWeatherResult *result)
{
    static struct AmiWeatherReport candidate;
    *result = amiweather_openmeteo_fetch_report_units(
        location, door_state.preferences.temperature_unit,
        door_state.preferences.wind_unit, &candidate);
    if (result->error != AMIWEATHER_OK) return 0;
    *report = candidate;
    return 1;
}

static void load_start_location(void)
{
    enum AmiWeatherPreferencesError error;
    amiweather_preferences_default(&door_state.preferences);
    error = amiweather_preferences_load_resolved(
        AMIWEATHER_ENV_PATH, AMIWEATHER_ENVARC_PATH, &door_state.preferences);
    if (error != AMIWEATHER_PREFS_OK && error != AMIWEATHER_PREFS_NOT_FOUND)
        amiweather_preferences_default(&door_state.preferences);
    if (door_state.preferences.has_default_location) {
        door_state.location = door_state.preferences.saved_locations[
            door_state.preferences.default_location_index];
        door_state.has_location = 1;
    }
}

static int refresh_report(const struct AmiWeatherDoorTerminal *terminal)
{
    struct AmiWeatherResult result;
    static struct AmiWeatherReport candidate;
    if (!door_state.has_location) {
        amiweather_door_write(terminal,
            "No default location configured.\nPress L to select a location.\n");
        return 0;
    }
    if (!fetch_location(&door_state.location, &candidate, &result)) {
        show_error(terminal, "Weather update", &result);
        return 0;
    }
    door_state.report = candidate;
    door_state.has_report = 1;
    return 1;
}

static int choose_location(const struct AmiWeatherDoorTerminal *terminal)
{
    static struct AmiWeatherLocationList results;
    static struct AmiWeatherReport candidate;
    char query[AMIWEATHER_DOOR_INPUT_SIZE];
    char line[256];
    int key;
    size_t i, selected;
    struct AmiWeatherResult result;

    amiweather_door_write(terminal, "\nLocation search: ");
    if (!read_line(terminal, query, sizeof(query))) return 0;
    if (!query[0] || (query[0] == '0' && query[1] == '\0')) return 1;
    result = amiweather_geocode_search(query, &results);
    if (result.error != AMIWEATHER_OK) {
        show_error(terminal, "Location search", &result);
        return 1;
    }
    if (!results.count) {
        amiweather_door_write(terminal, "No locations found.\n");
        return 1;
    }
    amiweather_door_write(terminal, "Select a location (1-8), or Q to cancel:\n");
    for (i = 0; i < results.count; ++i) {
        char display[AMIWEATHER_LOCATION_DISPLAY_SIZE];
        amiweather_location_format(&results.locations[i], display, sizeof(display));
        snprintf(line, sizeof(line), "%u. %s\n", (unsigned int)(i + 1), display);
        amiweather_door_write(terminal, line);
    }
    if (!terminal->read_key(terminal->context, &key)) return 0;
    if (key == 'q' || key == 'Q') return 1;
    if (key < '1' || key > '8') return 1;
    selected = (size_t)(key - '1');
    if (selected >= results.count) return 1;
    if (!fetch_location(&results.locations[selected], &candidate, &result)) {
        show_error(terminal, "Weather update", &result);
        return 1;
    }
    door_state.location = results.locations[selected];
    door_state.has_location = 1;
    door_state.report = candidate;
    door_state.has_report = 1;
    return 1;
}

static void render_view(const struct AmiWeatherDoorTerminal *terminal,
                        enum AmiWeatherDoorCommand command)
{
    if (!door_state.has_report) {
        amiweather_door_write(terminal, "\nNo weather report is available. Press R to retry or Q to return.\n");
        return;
    }
    switch (command) {
    case AMIWEATHER_DOOR_COMMAND_HOURLY:
        amiweather_door_render_hourly(terminal, &door_state.report);
        break;
    case AMIWEATHER_DOOR_COMMAND_DAILY:
        amiweather_door_write(terminal, "\nDaily Forecast\n---------------\n");
        amiweather_door_render_daily(terminal, &door_state.report, AMIWEATHER_FORECAST_DAYS);
        break;
    case AMIWEATHER_DOOR_COMMAND_SEVEN_DAY:
        amiweather_door_write(terminal, "\n7 Day Forecast\n---------------\n");
        amiweather_door_render_daily(terminal, &door_state.report, 7);
        break;
    case AMIWEATHER_DOOR_COMMAND_FOURTEEN_DAY:
        amiweather_door_write(terminal, "\n14 Day Forecast\n----------------\n");
        amiweather_door_render_daily(terminal, &door_state.report, AMIWEATHER_MAX_DAILY);
        break;
    case AMIWEATHER_DOOR_COMMAND_DETAILS:
        amiweather_door_render_details(terminal, &door_state.report);
        break;
    default:
        break;
    }
}

int amiweather_door_amiga_run(void)
{
    struct AmiWeatherDoorTerminal terminal;
    int key;
    enum AmiWeatherDoorCommand command;
    terminal.context = NULL;
    terminal.write = amiga_write;
    terminal.read_key = amiga_read_key;
    terminal.connected = amiga_connected;
    terminal.width = 80;
    load_start_location();
    if (door_state.has_location)
        (void)refresh_report(&terminal);
    if (door_state.has_report)
        amiweather_door_render_home(&terminal, &door_state.location, &door_state.report);
    else if (!door_state.has_location)
        amiweather_door_write(&terminal,
            "AmiWeather\n==========\nNo default location configured.\n"
            "Press L to select a location.\n");
    else
        amiweather_door_write(&terminal,
            "AmiWeather\n==========\nWeather unavailable.\n");
    for (;;) {
        amiweather_door_write(&terminal, "\n[H] Hourly [D] Daily [7] 7 day [A] 14 day [W] Details\n[L] Location [R] Refresh [Q] Return to BBS\n> ");
        if (!read_command(&terminal, &key)) return 0;
        command = amiweather_door_command(key);
        if (command == AMIWEATHER_DOOR_COMMAND_QUIT) return 0;
        if (command == AMIWEATHER_DOOR_COMMAND_REFRESH) {
            if (refresh_report(&terminal))
                amiweather_door_render_home(&terminal, &door_state.location, &door_state.report);
        } else if (command == AMIWEATHER_DOOR_COMMAND_LOCATION) {
            if (choose_location(&terminal) && door_state.has_report)
                amiweather_door_render_home(&terminal, &door_state.location, &door_state.report);
        } else if (command != AMIWEATHER_DOOR_COMMAND_NONE) {
            render_view(&terminal, command);
        } else {
            amiweather_door_write(&terminal, "Unknown command.\n");
        }
    }
    return 0;
}
