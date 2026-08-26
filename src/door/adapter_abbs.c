/* Thin Shell-door context adapter.  ABBS supplies these values as command
 * arguments from the documented C:get variables; no private ABBS ABI is used. */
#include <amiweather/door.h>
#include <string.h>
#include <stdlib.h>

int amiweather_abbs_parse_args(int argc, char **argv,
                               struct AmiWeatherAbbsContext *context)
{
    int i;
    if (!context) return 0;
    memset(context, 0, sizeof(*context));
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--abbs-user") == 0) {
            size_t length;
            if (++i >= argc) return 0;
            length = strlen(argv[i]);
            if (length == 0 || length >= sizeof(context->full_name)) return 0;
            memcpy(context->full_name, argv[i], length + 1);
            context->has_full_name = 1;
        } else if (strcmp(argv[i], "--abbs-node") == 0) {
            char *end;
            long node;
            if (++i >= argc || argv[i][0] == '-' || argv[i][0] == '\0') return 0;
            node = strtol(argv[i], &end, 10);
            if (*end != '\0' || node < 0 || node > 9999) return 0;
            context->node_number = node;
            context->has_node_number = 1;
        }
    }
    return 1;
}
