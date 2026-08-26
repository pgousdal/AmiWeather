#ifndef AMIWEATHER_AREXX_H
#define AMIWEATHER_AREXX_H

#include <stddef.h>
#include <amiweather/openmeteo.h>

#define AMIWEATHER_AREXX_PORT "AMIWEATHER"
#define AMIWEATHER_AREXX_COMMAND_SIZE 256
#define AMIWEATHER_AREXX_RESULT_SIZE 256

enum AmiWeatherARexxCode {
    AMIWEATHER_AREXX_OK = 0,
    AMIWEATHER_AREXX_UNKNOWN_COMMAND = 10,
    AMIWEATHER_AREXX_INVALID_ARGUMENTS = 11,
    AMIWEATHER_AREXX_NO_REPORT = 12,
    AMIWEATHER_AREXX_NO_LOCATION = 13,
    AMIWEATHER_AREXX_AMBIGUOUS_LOCATION = 14,
    AMIWEATHER_AREXX_GEOCODING_FAILED = 15,
    AMIWEATHER_AREXX_SAVE_FAILED = 16,
    AMIWEATHER_AREXX_UNAVAILABLE = 17
};

struct AmiWeatherARexxState {
    struct AmiWeatherPreferences *preferences;
    struct AmiWeatherLocation *location;
    struct AmiWeatherReport *report;
    int *has_location;
    int *has_report;
    int *refresh_active;
    struct AmiWeatherResult *last_result;
};

struct AmiWeatherARexxOperations {
    struct AmiWeatherResult (*search)(const char *, struct AmiWeatherLocationList *);
    struct AmiWeatherResult (*fetch)(const struct AmiWeatherLocation *,
        enum AmiWeatherTemperatureUnit, enum AmiWeatherWindUnit,
        struct AmiWeatherReport *);
    enum AmiWeatherPreferencesError (*save)(struct AmiWeatherPreferences *);
};

int amiweather_arexx_command(const char *command, size_t command_length,
    struct AmiWeatherARexxState *state,
    const struct AmiWeatherARexxOperations *operations,
    char *result, size_t result_size);

#ifdef AMIWEATHER_AMIGA
#include <exec/types.h>
struct AmiWeatherARexxHost;
struct AmiWeatherARexxHost *amiweather_arexx_host_open(void);
ULONG amiweather_arexx_host_signal(const struct AmiWeatherARexxHost *host);
void amiweather_arexx_host_process(struct AmiWeatherARexxHost *host,
    struct AmiWeatherARexxState *state,
    const struct AmiWeatherARexxOperations *operations);
void amiweather_arexx_host_close(struct AmiWeatherARexxHost *host);
#endif

#endif
