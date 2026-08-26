#ifndef AMIWEATHER_DOOR_H
#define AMIWEATHER_DOOR_H

#include <stddef.h>
#include <amiweather/location.h>
#include <amiweather/weather.h>

struct AmiWeatherDoorTerminal {
    void *context;
    int (*write)(void *context, const char *text, size_t length);
    int (*read_key)(void *context, int *key);
    int (*connected)(void *context);
    unsigned int width;
};

struct AmiWeatherDoorAdapter {
    struct AmiWeatherDoorTerminal terminal;
    int (*return_to_bbs)(void *context);
};

enum AmiWeatherDoorCommand {
    AMIWEATHER_DOOR_COMMAND_NONE = 0,
    AMIWEATHER_DOOR_COMMAND_HOURLY,
    AMIWEATHER_DOOR_COMMAND_DAILY,
    AMIWEATHER_DOOR_COMMAND_SEVEN_DAY,
    AMIWEATHER_DOOR_COMMAND_FOURTEEN_DAY,
    AMIWEATHER_DOOR_COMMAND_DETAILS,
    AMIWEATHER_DOOR_COMMAND_LOCATION,
    AMIWEATHER_DOOR_COMMAND_REFRESH,
    AMIWEATHER_DOOR_COMMAND_QUIT
};

int amiweather_door_write(const struct AmiWeatherDoorTerminal *terminal,
                          const char *text);
int amiweather_door_render_home(const struct AmiWeatherDoorTerminal *terminal,
                                const struct AmiWeatherLocation *location,
                                const struct AmiWeatherReport *report);
int amiweather_door_render_daily(const struct AmiWeatherDoorTerminal *terminal,
                                 const struct AmiWeatherReport *report,
                                 size_t limit);
int amiweather_door_render_hourly(const struct AmiWeatherDoorTerminal *terminal,
                                  const struct AmiWeatherReport *report);
int amiweather_door_render_details(const struct AmiWeatherDoorTerminal *terminal,
                                   const struct AmiWeatherReport *report);
enum AmiWeatherDoorCommand amiweather_door_command(int key);

#endif
