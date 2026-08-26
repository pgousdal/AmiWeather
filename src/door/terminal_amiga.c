#include <amiweather/door.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

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

int amiweather_door_amiga_run(int argc, char **argv)
{
    struct AmiWeatherDoorTerminal terminal;
    struct AmiWeatherAbbsContext context;
    terminal.context = NULL;
    terminal.write = amiga_write;
    terminal.read_key = amiga_read_key;
    terminal.connected = amiga_connected;
    terminal.width = 80;
    if (!amiweather_abbs_parse_args(argc, argv, &context))
        return 20;
    amiweather_door_write(&terminal, "AmiWeatherDoor\n===============\n");
    if (context.has_full_name) {
        amiweather_door_write(&terminal, "Welcome, ");
        amiweather_door_write(&terminal, context.full_name);
        amiweather_door_write(&terminal, "\n");
    }
    amiweather_door_write(&terminal, "ABBS Shell-door foundation; weather session wiring follows in M8.2.\n");
    return 0;
}
