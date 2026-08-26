#ifndef AMIWEATHER_LOCATION_H
#define AMIWEATHER_LOCATION_H

#include <stddef.h>

#define AMIWEATHER_LOCATION_NAME_SIZE 96
#define AMIWEATHER_LOCATION_ADMIN1_SIZE 96
#define AMIWEATHER_LOCATION_COUNTRY_SIZE 96
#define AMIWEATHER_LOCATION_COUNTRY_CODE_SIZE 4
#define AMIWEATHER_LOCATION_TIMEZONE_SIZE 64
#define AMIWEATHER_LOCATION_DISPLAY_SIZE 256
#define AMIWEATHER_LOCATION_MAX_RESULTS 8

struct AmiWeatherLocation {
    char name[AMIWEATHER_LOCATION_NAME_SIZE];
    char admin1[AMIWEATHER_LOCATION_ADMIN1_SIZE];
    char country[AMIWEATHER_LOCATION_COUNTRY_SIZE];
    char country_code[AMIWEATHER_LOCATION_COUNTRY_CODE_SIZE];
    double latitude;
    double longitude;
    char timezone[AMIWEATHER_LOCATION_TIMEZONE_SIZE];
};

struct AmiWeatherLocationList {
    struct AmiWeatherLocation locations[AMIWEATHER_LOCATION_MAX_RESULTS];
    size_t count;
};

void amiweather_location_default(struct AmiWeatherLocation *location);
void amiweather_location_format(const struct AmiWeatherLocation *location,
                                char *buffer, size_t size);

#endif
