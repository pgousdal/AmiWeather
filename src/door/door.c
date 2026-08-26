#include <amiweather/door.h>

#include <stdio.h>
#include <string.h>

int amiweather_door_write(const struct AmiWeatherDoorTerminal *terminal,
                          const char *text)
{
    size_t length;
    if (!terminal || !terminal->write || !text) return 0;
    length = strlen(text);
    return terminal->write(terminal->context, text, length) == (int)length;
}

static void tenths(char *out, size_t size, double value)
{
    long v = value >= 0.0 ? (long)(value * 10.0 + 0.5) : (long)(value * 10.0 - 0.5);
    unsigned long m = v < 0 ? (unsigned long)(-(v + 1)) + 1UL : (unsigned long)v;
    snprintf(out, size, "%s%lu.%lu", v < 0 ? "-" : "", m / 10UL, m % 10UL);
}

int amiweather_door_render_home(const struct AmiWeatherDoorTerminal *terminal,
                                const struct AmiWeatherLocation *location,
                                const struct AmiWeatherReport *report)
{
    char line[512], temperature[24], apparent[24], wind[24], humidity[24], place[128];
    if (!terminal || !location || !report) return 0;
    amiweather_location_format(location, place, sizeof(place));
    tenths(temperature, sizeof(temperature), report->current.temperature_c);
    tenths(apparent, sizeof(apparent), report->current.apparent_temperature_c);
    tenths(wind, sizeof(wind), report->current.wind_speed_kmh);
    tenths(humidity, sizeof(humidity), report->current.relative_humidity_percent);
    amiweather_door_write(terminal, "AmiWeather\n==========\n\n");
    snprintf(line, sizeof(line), "%s\n\nCurrent Conditions\n------------------\n%s\nTemperature .... %s %s\nFeels like ..... %s %s\nHumidity ....... %s %%\nWind ........... %s %s %s\n\nForecast\n--------\n",
             place, amiweather_wmo_description(report->current.weather_code), temperature,
             amiweather_temperature_unit_label(report->temperature_unit), apparent,
             amiweather_temperature_unit_label(report->temperature_unit),
             humidity, wind,
             amiweather_wind_unit_label(report->wind_unit),
             report->current.has_wind_direction ? amiweather_wind_direction(report->current.wind_direction_degrees) : "");
    amiweather_door_write(terminal, line);
    amiweather_door_render_daily(terminal, report, AMIWEATHER_FORECAST_DAYS);
    amiweather_door_write(terminal, "\n[H] Hourly  [D] Daily  [7] 7 day  [A] 14 day  [W] Details\n[L] Location  [R] Refresh  [Q] Return to BBS\n");
    return 1;
}

int amiweather_door_render_daily(const struct AmiWeatherDoorTerminal *terminal,
                                 const struct AmiWeatherReport *report, size_t limit)
{
    char line[160], min[24], max[24], date[AMIWEATHER_COMPACT_DATE_SIZE];
    size_t i, count;
    if (!terminal || !report) return 0;
    count = report->forecast.count < limit ? report->forecast.count : limit;
    for (i = 0; i < count; ++i) {
        const struct AmiWeatherDaily *day = &report->forecast.days[i];
        amiweather_format_forecast_date(day->date, date, sizeof(date));
        tenths(min, sizeof(min), day->temperature_min_c); tenths(max, sizeof(max), day->temperature_max_c);
        snprintf(line, sizeof(line), "%-6s %-18s %s / %s %s",
                 date, amiweather_wmo_compact_description(day->weather_code), min, max,
                 amiweather_temperature_unit_label(report->temperature_unit));
        if (day->has_precipitation_probability)
            snprintf(line + strlen(line), sizeof(line) - strlen(line), " %d%%", day->precipitation_probability);
        snprintf(line + strlen(line), sizeof(line) - strlen(line), "\n");
        amiweather_door_write(terminal, line);
    }
    return 1;
}

int amiweather_door_render_hourly(const struct AmiWeatherDoorTerminal *terminal,
                                  const struct AmiWeatherReport *report)
{
    char line[160], temperature[24], time[AMIWEATHER_COMPACT_TIME_SIZE];
    size_t i;
    if (!terminal || !report) return 0;
    amiweather_door_write(terminal, "Hourly\n-------\n");
    for (i = 0; i < report->hourly.count; ++i) {
        const struct AmiWeatherHourly *entry = &report->hourly.entries[i];
        amiweather_format_hourly_time(entry->time, time, sizeof(time)); tenths(temperature, sizeof(temperature), entry->temperature);
        snprintf(line, sizeof(line), "%s  %-18s %s %s%s\n", time, amiweather_wmo_compact_description(entry->weather_code), temperature,
                 amiweather_temperature_unit_label(report->temperature_unit), entry->has_precipitation_probability ? "" : "");
        if (entry->has_precipitation_probability)
            snprintf(line, sizeof(line), "%s  %-18s %s %s %d%%\n", time, amiweather_wmo_compact_description(entry->weather_code), temperature, amiweather_temperature_unit_label(report->temperature_unit), entry->precipitation_probability);
        amiweather_door_write(terminal, line);
    }
    return 1;
}

int amiweather_door_render_details(const struct AmiWeatherDoorTerminal *terminal,
                                   const struct AmiWeatherReport *report)
{
    char line[160], value[24];
    const struct AmiWeatherCurrent *c;
    if (!terminal || !report) return 0;
    c = &report->current; amiweather_door_write(terminal, "Weather Details\n----------------\n");
    tenths(value, sizeof(value), c->temperature_c); snprintf(line, sizeof(line), "Temperature       %s %s\n", value, amiweather_temperature_unit_label(report->temperature_unit)); amiweather_door_write(terminal, line);
    if (c->has_wind_direction) { snprintf(line, sizeof(line), "Wind direction    %s\n", amiweather_wind_direction(c->wind_direction_degrees)); amiweather_door_write(terminal, line); }
    if (c->has_surface_pressure) { tenths(value, sizeof(value), c->surface_pressure); snprintf(line, sizeof(line), "Pressure          %s hPa\n", value); amiweather_door_write(terminal, line); }
    return 1;
}

enum AmiWeatherDoorCommand amiweather_door_command(int key)
{
    switch (key) {
    case 'h': case 'H': return AMIWEATHER_DOOR_COMMAND_HOURLY;
    case 'd': case 'D': return AMIWEATHER_DOOR_COMMAND_DAILY;
    case '7': return AMIWEATHER_DOOR_COMMAND_SEVEN_DAY;
    case 'a': case 'A': return AMIWEATHER_DOOR_COMMAND_FOURTEEN_DAY;
    case 'w': case 'W': return AMIWEATHER_DOOR_COMMAND_DETAILS;
    case 'l': case 'L': return AMIWEATHER_DOOR_COMMAND_LOCATION;
    case 'r': case 'R': return AMIWEATHER_DOOR_COMMAND_REFRESH;
    case 'q': case 'Q': return AMIWEATHER_DOOR_COMMAND_QUIT;
    default: return AMIWEATHER_DOOR_COMMAND_NONE;
    }
}
