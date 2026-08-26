#include <amiweather/door.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "check failed: %s\n", #x); ++failures; } } while (0)
struct Sink { char data[8192]; size_t length; };
static int sink_write(void *context, const char *text, size_t length)
{ struct Sink *sink = context; if (length > sizeof(sink->data)-sink->length-1) return 0; memcpy(sink->data+sink->length,text,length); sink->length += length; sink->data[sink->length]=0; return (int)length; }
static void test_render(void)
{
    struct Sink sink = {{0},0}; struct AmiWeatherDoorTerminal terminal = {&sink,sink_write,NULL,NULL,80};
    struct AmiWeatherLocation location; struct AmiWeatherReport report;
    memset(&report,0,sizeof(report)); amiweather_location_default(&location);
    report.temperature_unit = AMIWEATHER_TEMPERATURE_CELSIUS; report.wind_unit = AMIWEATHER_WIND_KMH;
    report.current.temperature_c=-2.5; report.current.apparent_temperature_c=-3.0; report.current.relative_humidity_percent=62; report.current.wind_speed_kmh=12.3; report.current.weather_code=100;
    report.forecast.count=1; strcpy(report.forecast.days[0].date,"2026-12-19"); report.forecast.days[0].temperature_min_c=-10.2; report.forecast.days[0].temperature_max_c=1.5; report.forecast.days[0].weather_code=63;
    CHECK(amiweather_door_render_home(&terminal,&location,&report)); CHECK(strstr(sink.data,"-2.5")!=NULL); CHECK(strstr(sink.data,"Unknown")!=NULL); CHECK(strstr(sink.data,"Forecast")!=NULL);
    CHECK(amiweather_door_command('7') == AMIWEATHER_DOOR_COMMAND_SEVEN_DAY); CHECK(amiweather_door_command('q') == AMIWEATHER_DOOR_COMMAND_QUIT);
}
int main(void) { test_render(); if (failures) return 1; puts("AmiWeather door tests passed"); return 0; }
