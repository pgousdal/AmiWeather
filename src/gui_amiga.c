#include <amiweather/openmeteo.h>
#include <amiweather/preferences.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <exec/lists.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#define GID_REFRESH 1
#define GID_CHANGE 2
#define GID_LOCATIONS 3
#define GID_PREFERENCES 4
#define GID_HOURLY 5
#define GID_SEARCH_TEXT 10
#define GID_SEARCH 11
#define GID_RESULTS 12
#define GID_USE 13
#define GID_CANCEL 14
#define SEARCH_TEXT_SIZE 80
#define GID_SAVED_LIST 20
#define GID_SAVED_USE 21
#define GID_SAVED_ADD 22
#define GID_SAVED_REMOVE 23
#define GID_SAVED_DEFAULT 24
#define GID_SAVED_CLOSE 25
#define GID_PREF_TEMPERATURE 30
#define GID_PREF_WIND 31
#define GID_PREF_SAVE 32
#define GID_PREF_CANCEL 33

#define AMIWEATHER_ENV_PATH "ENV:AmiWeather/preferences"
#define AMIWEATHER_ENVARC_PATH "ENVARC:AmiWeather/preferences"

struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;
struct GfxBase *GfxBase;

enum ViewState {
    VIEW_LOADING = 0,
    VIEW_WEATHER,
    VIEW_ERROR
};

struct ApplicationState {
    enum ViewState view;
    struct AmiWeatherReport report;
    struct AmiWeatherResult result;
    struct AmiWeatherLocation location;
    struct AmiWeatherPreferences preferences;
    BOOL has_report;
};

struct WindowLayout {
    WORD width;
    WORD height;
    WORD left;
    WORD content_top;
    WORD baseline_step;
    WORD gadget_left;
    WORD gadget_top;
    WORD gadget_width;
    WORD gadget_height;
    WORD change_left;
    WORD hourly_left;
};

static void calculate_layout(const struct Screen *screen,
                             struct WindowLayout *layout)
{
    WORD font_height = screen->Font ? screen->Font->ta_YSize : 8;
    WORD available_width = screen->Width - 12;
    WORD available_height = screen->Height - 12;
    WORD desired_height;

    layout->width = available_width < 500 ? available_width : 500;
    layout->left = screen->WBorLeft + 12;
    layout->content_top = screen->WBorTop + font_height + 10;
    layout->baseline_step = font_height + 4;
    layout->gadget_width = 76;
    layout->gadget_height = font_height + 10;
    desired_height = layout->content_top + 15 * layout->baseline_step +
                     layout->gadget_height + screen->WBorBottom + 10;
    layout->height = desired_height < available_height
        ? desired_height : available_height;
    layout->gadget_left = layout->left;
    layout->change_left = layout->left + layout->gadget_width + 8;
    layout->hourly_left = layout->change_left + layout->gadget_width + 8;
    layout->gadget_top = layout->height - screen->WBorBottom -
                         layout->gadget_height - 6;
}

enum MenuAction {
    MENU_REFRESH = 1,
    MENU_ABOUT,
    MENU_QUIT,
    MENU_CHANGE,
    MENU_LOCATIONS,
    MENU_HOURLY,
    MENU_PREFERENCES
};

static struct Menu *create_application_menus(APTR visual_info)
{
    static const struct NewMenu definitions[] = {
        { NM_TITLE, (CONST_STRPTR)"Project", NULL, 0, 0, NULL },
        { NM_ITEM, (CONST_STRPTR)"Refresh", (CONST_STRPTR)"R", 0, 0,
          (APTR)MENU_REFRESH },
        { NM_ITEM, (CONST_STRPTR)"About...", (CONST_STRPTR)"A", 0, 0,
          (APTR)MENU_ABOUT },
        { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
        { NM_ITEM, (CONST_STRPTR)"Quit", (CONST_STRPTR)"Q", 0, 0,
          (APTR)MENU_QUIT },
        { NM_TITLE, (CONST_STRPTR)"Location", NULL, 0, 0, NULL },
        { NM_ITEM, (CONST_STRPTR)"Change...", (CONST_STRPTR)"C", 0, 0,
          (APTR)MENU_CHANGE },
        { NM_ITEM, (CONST_STRPTR)"Saved Locations...", (CONST_STRPTR)"L", 0, 0,
          (APTR)MENU_LOCATIONS },
        { NM_TITLE, (CONST_STRPTR)"Forecast", NULL, 0, 0, NULL },
        { NM_ITEM, (CONST_STRPTR)"Hourly...", (CONST_STRPTR)"H", 0, 0,
          (APTR)MENU_HOURLY },
        { NM_TITLE, (CONST_STRPTR)"Settings", NULL, 0, 0, NULL },
        { NM_ITEM, (CONST_STRPTR)"Preferences...", (CONST_STRPTR)"P", 0, 0,
          (APTR)MENU_PREFERENCES },
        { NM_END, NULL, NULL, 0, 0, NULL }
    };
    struct Menu *menus = CreateMenus(definitions, TAG_DONE);
    if (!menus || !LayoutMenus(menus, visual_info, TAG_DONE)) {
        if (menus) FreeMenus(menus);
        return NULL;
    }
    return menus;
}

static void show_about(struct Window *parent)
{
    struct EasyStruct about;
    char text[320];
    snprintf(text, sizeof(text),
             "AmiWeather %s\\n\\nNative weather client for AmigaOS\\n\\n"
             "Weather data: Open-Meteo\\nHTTP/API support: AmiAPI\\n"
             "HTTPS: AmiSSL\\n\\nCopyright (c) 2026 Per Gustav Ousdal\\n"
             "Licensed under the MIT License", AMIWEATHER_VERSION);
    about.es_Flags = 0;
    about.es_Title = (UBYTE *)"About AmiWeather";
    about.es_TextFormat = (UBYTE *)text;
    about.es_GadgetFormat = (UBYTE *)"OK";
    EasyRequest(parent, &about, NULL);
}

static void draw_clipped(struct RastPort *rp, WORD x, WORD y,
                         const char *text, WORD max_width)
{
    size_t length = strlen(text);

    while (length > 0 && TextLength(rp, (STRPTR)text, (ULONG)length) > max_width)
        --length;
    if (length > 0) {
        Move(rp, x, y);
        Text(rp, (STRPTR)text, (ULONG)length);
    }
}

/* Small, compiled-in line art: it uses only the screen's detail pen and
 * remains legible on ECS/AGA/RTG palettes without external bitmap files. */
static void draw_weather_icon(struct RastPort *rp, WORD x, WORD y,
                              enum AmiWeatherIcon icon, WORD scale)
{
    WORD s = scale < 1 ? 1 : scale;
    if (icon == AMIWEATHER_ICON_CLEAR || icon == AMIWEATHER_ICON_PARTLY_CLOUDY) {
        RectFill(rp, x + 7*s, y + 2*s, x + 10*s, y + 5*s);
        Move(rp, x + 8*s, y); Draw(rp, x + 8*s, y + 7*s);
        Move(rp, x + 5*s, y + 1*s); Draw(rp, x + 11*s, y + 7*s);
        Move(rp, x + 11*s, y + 1*s); Draw(rp, x + 5*s, y + 7*s);
    }
    if (icon == AMIWEATHER_ICON_PARTLY_CLOUDY ||
        icon == AMIWEATHER_ICON_CLOUDY || icon == AMIWEATHER_ICON_FOG ||
        icon == AMIWEATHER_ICON_DRIZZLE || icon == AMIWEATHER_ICON_RAIN ||
        icon == AMIWEATHER_ICON_SHOWERS || icon == AMIWEATHER_ICON_SNOW ||
        icon == AMIWEATHER_ICON_SNOW_SHOWERS ||
        icon == AMIWEATHER_ICON_THUNDERSTORM || icon == AMIWEATHER_ICON_UNKNOWN) {
        RectFill(rp, x + 3*s, y + 7*s, x + 18*s, y + 12*s);
        RectFill(rp, x + 6*s, y + 5*s, x + 14*s, y + 8*s);
        Move(rp, x + 3*s, y + 12*s); Draw(rp, x + 18*s, y + 12*s);
    }
    if (icon == AMIWEATHER_ICON_FOG) {
        Move(rp, x + 1*s, y + 4*s); Draw(rp, x + 20*s, y + 4*s);
        Move(rp, x + 1*s, y + 15*s); Draw(rp, x + 20*s, y + 15*s);
    } else if (icon == AMIWEATHER_ICON_DRIZZLE || icon == AMIWEATHER_ICON_RAIN ||
               icon == AMIWEATHER_ICON_SHOWERS) {
        Move(rp, x + 6*s, y + 14*s); Draw(rp, x + 5*s, y + 17*s);
        Move(rp, x + 11*s, y + 14*s); Draw(rp, x + 10*s, y + 17*s);
        Move(rp, x + 16*s, y + 14*s); Draw(rp, x + 15*s, y + 17*s);
    } else if (icon == AMIWEATHER_ICON_SNOW || icon == AMIWEATHER_ICON_SNOW_SHOWERS) {
        Move(rp, x + 7*s, y + 14*s); Draw(rp, x + 7*s, y + 18*s);
        Move(rp, x + 5*s, y + 16*s); Draw(rp, x + 9*s, y + 16*s);
        Move(rp, x + 12*s, y + 14*s); Draw(rp, x + 12*s, y + 18*s);
        Move(rp, x + 10*s, y + 16*s); Draw(rp, x + 14*s, y + 16*s);
    } else if (icon == AMIWEATHER_ICON_THUNDERSTORM) {
        Move(rp, x + 11*s, y + 13*s); Draw(rp, x + 8*s, y + 18*s);
        Draw(rp, x + 12*s, y + 15*s); Draw(rp, x + 9*s, y + 15*s);
    }
}

static void draw_line_at(struct RastPort *rp, const struct WindowLayout *layout,
                         WORD row, WORD x, const char *text)
{
    WORD y = layout->content_top + row * layout->baseline_step;
    draw_clipped(rp, x, y, text, layout->width - x - 10);
}

static void draw_line(struct RastPort *rp, const struct WindowLayout *layout,
                      WORD row, const char *text)
{
    WORD y = layout->content_top + row * layout->baseline_step;
    draw_clipped(rp, layout->left, y, text,
                 layout->width - layout->left - 10);
}

static const char *visible_error_detail(const struct AmiWeatherResult *result)
{
    if ((result->error == AMIWEATHER_ERR_NETWORK ||
         result->error == AMIWEATHER_ERR_HTTP_STATUS) &&
        result->diagnostic[0])
        return result->diagnostic;
    return amiweather_error_string(result->error);
}

/*
 * Avoid printf-family floating-point formatting on classic AmigaOS.
 * Keep the weather domain values as doubles, but convert display values
 * to integer tenths and format them with integer conversions only.
 */
static void format_tenths(
    char *buffer,
    size_t buffer_size,
    double value)
{
    long tenths;
    unsigned long magnitude;
    unsigned long whole;
    unsigned long fraction;

    if (!buffer || buffer_size == 0)
        return;

    /*
     * Round to the nearest tenth without requiring libm.
     */
    if (value >= 0.0)
        tenths = (long)(value * 10.0 + 0.5);
    else
        tenths = (long)(value * 10.0 - 0.5);

    if (tenths < 0)
        magnitude = (unsigned long)(-(tenths + 1L)) + 1UL;
    else
        magnitude = (unsigned long)tenths;

    whole = magnitude / 10UL;
    fraction = magnitude % 10UL;

    if (tenths < 0) {
        snprintf(
            buffer,
            buffer_size,
            "-%lu.%lu",
            whole,
            fraction
        );
    } else {
        snprintf(
            buffer,
            buffer_size,
            "%lu.%lu",
            whole,
            fraction
        );
    }
}

static void draw_content(struct Window *window,
                         const struct WindowLayout *layout,
                         const struct ApplicationState *application)
{
    struct RastPort *rp = window->RPort;
    char line[96];
    ULONG supported_style;
    WORD content_bottom = layout->gadget_top - 5;

    /*
     * Do not rely on the RastPort drawing state left behind by
     * Intuition or GadTools.  In particular, JAM2 would paint the
     * text background with BPen, which can make the foreground text
     * disappear into the content background on classic Workbench
     * palettes.
     */
    SetDrMd(rp, JAM1);

    SetAPen(rp, window->WScreen->BlockPen);
    RectFill(rp, window->BorderLeft, window->BorderTop,
             window->Width - window->BorderRight - 1, content_bottom);

    SetAPen(rp, window->WScreen->DetailPen);

    supported_style = AskSoftStyle(rp);
    SetSoftStyle(rp, FSF_BOLD, supported_style);
    amiweather_location_format(&application->location, line, sizeof(line));
    draw_line(rp, layout, 0, line);
    SetSoftStyle(rp, FS_NORMAL, supported_style);

    if (!application->has_report && application->view == VIEW_LOADING) {
        draw_line(rp, layout, 2, "Fetching weather...");
        return;
    }
    if (!application->has_report && application->view == VIEW_ERROR) {
        draw_line(rp, layout, 2, "Unable to update weather");
        draw_line(rp, layout, 4, visible_error_detail(&application->result));
        return;
    }

    {
        char value[32];

        format_tenths(
            value,
            sizeof(value),
            application->report.current.temperature_c
        );
        snprintf(line, sizeof(line), "%s \260%s", value,
                 amiweather_temperature_unit_label(application->preferences.temperature_unit));
    }

    SetSoftStyle(rp, FSF_BOLD, supported_style);
    draw_line_at(rp, layout, 2, layout->left + 32, line);
    draw_weather_icon(rp, layout->left, layout->content_top + 2 * layout->baseline_step - 12,
                      amiweather_wmo_icon(application->report.current.weather_code), 1);
    SetSoftStyle(rp, FS_NORMAL, supported_style);
    draw_line_at(rp, layout, 3, layout->left + 32,
              amiweather_wmo_description(application->report.current.weather_code));

    {
        char value[32];

        format_tenths(
            value,
            sizeof(value),
            application->report.current.apparent_temperature_c
        );
        snprintf(line, sizeof(line), "Feels like %s \260%s", value,
                 amiweather_temperature_unit_label(application->preferences.temperature_unit));
    }
    draw_line(rp, layout, 4, line);

    snprintf(
        line,
        sizeof(line),
        "Humidity %ld%%",
        (long)(application->report.current.relative_humidity_percent + 0.5)
    );
    draw_line(rp, layout, 5, line);

    {
        char value[32];

        format_tenths(
            value,
            sizeof(value),
            application->report.current.wind_speed_kmh
        );
        snprintf(line, sizeof(line), "Wind %s %s", value,
                 amiweather_wind_unit_label(application->preferences.wind_unit));
    }
    draw_line(rp, layout, 6, line);

    SetSoftStyle(rp, FSF_BOLD, supported_style);
    draw_line(rp, layout, 8, "Forecast");
    SetSoftStyle(rp, FS_NORMAL, supported_style);
    {
        size_t i;
        for (i = 0; i < application->report.forecast.count; ++i) {
            const struct AmiWeatherDaily *day =
                &application->report.forecast.days[i];
            char date[AMIWEATHER_COMPACT_DATE_SIZE];
            char minimum[24];
            char maximum[24];
            const char *condition =
                amiweather_wmo_compact_description(day->weather_code);
            amiweather_format_forecast_date(day->date, date, sizeof(date));
            format_tenths(minimum, sizeof(minimum), day->temperature_min_c);
            format_tenths(maximum, sizeof(maximum), day->temperature_max_c);
            if (layout->width >= 460 && day->has_precipitation_probability)
                snprintf(line, sizeof(line), "%-6s %-18s %3d%%  %s / %s \260%s",
                         date, condition, day->precipitation_probability,
                         minimum, maximum,
                         amiweather_temperature_unit_label(application->preferences.temperature_unit));
            else
                snprintf(line, sizeof(line), "%-6s %-18s %s / %s \260%s",
                         date, condition, minimum, maximum,
                         amiweather_temperature_unit_label(application->preferences.temperature_unit));
            draw_weather_icon(rp, layout->left,
                              layout->content_top + (WORD)(9 + i) * layout->baseline_step - 10,
                              amiweather_wmo_icon(day->weather_code), 1);
            draw_line_at(rp, layout, (WORD)(9 + i), layout->left + 24, line);
        }
    }
    if (application->view == VIEW_LOADING)
        draw_line(rp, layout, 14, "Updating...");
    else if (application->view == VIEW_ERROR) {
        snprintf(line, sizeof(line), "Update failed: %s",
                 visible_error_detail(&application->result));
        draw_line(rp, layout, 14, line);
    }
}

static void fetch_weather(struct Window *window,
                          const struct WindowLayout *layout,
                          struct ApplicationState *application)
{
    struct AmiWeatherReport updated;
    struct AmiWeatherResult result;

    application->view = VIEW_LOADING;
    draw_content(window, layout, application);
    result = amiweather_openmeteo_fetch_report_units(
        &application->location, application->preferences.temperature_unit,
        application->preferences.wind_unit, &updated);
    application->result = result;
    if (result.error == AMIWEATHER_OK) {
        application->report = updated;
        application->has_report = TRUE;
        application->view = VIEW_WEATHER;
    } else {
        application->view = VIEW_ERROR;
    }
    draw_content(window, layout, application);
}

static void show_result_requester(struct Window *parent, const char *title,
                                  const struct AmiWeatherResult *result)
{
    struct EasyStruct request;
    const char *detail = result->diagnostic[0]
        ? result->diagnostic : visible_error_detail(result);
    request.es_StructSize = sizeof(request);
    request.es_Flags = 0;
    request.es_Title = (STRPTR)title;
    request.es_TextFormat = (STRPTR)detail;
    request.es_GadgetFormat = (STRPTR)"OK";
    EasyRequestArgs(parent, &request, NULL, NULL);
}

static void initialize_list(struct List *list)
{
    list->lh_Head = (struct Node *)&list->lh_Tail;
    list->lh_Tail = NULL;
    list->lh_TailPred = (struct Node *)&list->lh_Head;
}

static BOOL search_location(struct Screen *screen, APTR visual_info,
                            struct AmiWeatherLocation *selected)
{
    struct Window *window = NULL;
    struct Gadget *gadgets = NULL;
    struct Gadget *gadget;
    struct Gadget *list_gadget = NULL;
    struct Gadget *string_gadget = NULL;
    struct NewGadget ng;
    struct List labels;
    struct Node nodes[AMIWEATHER_LOCATION_MAX_RESULTS];
    char display[AMIWEATHER_LOCATION_MAX_RESULTS][AMIWEATHER_LOCATION_DISPLAY_SIZE];
    char search_text[SEARCH_TEXT_SIZE];
    struct AmiWeatherLocationList results;
    struct IntuiMessage *message;
    ULONG class;
    UWORD code;
    UWORD gadget_id;
    WORD font_height = screen->Font ? screen->Font->ta_YSize : 8;
    WORD width = screen->Width - 20 < 440 ? screen->Width - 20 : 440;
    WORD height = screen->Height - 20 < 210 ? screen->Height - 20 : 210;
    WORD field_top = screen->WBorTop + font_height + 10;
    WORD button_height = font_height + 8;
    BOOL running = TRUE;
    BOOL accepted = FALSE;
    LONG selected_index = -1;
    size_t i;

    memset(search_text, 0, sizeof(search_text));
    memset(&results, 0, sizeof(results));
    initialize_list(&labels);
    gadget = CreateContext(&gadgets);
    if (!gadget)
        goto cleanup;
#define NEW_GADGET(left, top, gadget_width, gadget_height, text, id) do { \
    memset(&ng, 0, sizeof(ng)); ng.ng_LeftEdge = (left); ng.ng_TopEdge = (top); \
    ng.ng_Width = (gadget_width); ng.ng_Height = (gadget_height); \
    ng.ng_GadgetText = (CONST_STRPTR)(text); ng.ng_GadgetID = (id); \
    ng.ng_VisualInfo = visual_info; ng.ng_TextAttr = screen->Font; \
} while (0)
    NEW_GADGET(12, field_top, width - 112, button_height, NULL, GID_SEARCH_TEXT);
    gadget = CreateGadget(STRING_KIND, gadget, &ng,
                          GTST_String, (ULONG)search_text,
                          GTST_MaxChars, SEARCH_TEXT_SIZE - 1, TAG_DONE);
    if (!gadget) goto cleanup;
    string_gadget = gadget;
    NEW_GADGET(width - 92, field_top, 80, button_height, "_Search", GID_SEARCH);
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
    NEW_GADGET(12, field_top + button_height + 6, width - 24,
               height - field_top - 2 * button_height - 24,
               NULL, GID_RESULTS);
    gadget = CreateGadget(LISTVIEW_KIND, gadget, &ng,
                          GTLV_Labels, (ULONG)&labels,
                          GTLV_ShowSelected, NULL, TAG_DONE);
    if (!gadget) goto cleanup;
    list_gadget = gadget;
    NEW_GADGET(12, height - button_height - screen->WBorBottom - 5,
               80, button_height, "_Use", GID_USE);
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
    NEW_GADGET(100, height - button_height - screen->WBorBottom - 5,
               80, button_height, "_Cancel", GID_CANCEL);
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
#undef NEW_GADGET
    window = OpenWindowTags(NULL, WA_Title, (ULONG)"Location search",
        WA_CustomScreen, (ULONG)screen, WA_Width, (ULONG)width,
        WA_Height, (ULONG)height, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_Gadgets, (ULONG)gadgets,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW |
                  IDCMP_VANILLAKEY, TAG_DONE);
    if (!window) goto cleanup;
    GT_RefreshWindow(window, NULL);
    ActivateGadget(gadgets->NextGadget, window, NULL);
    while (running) {
        Wait(1UL << window->UserPort->mp_SigBit);
        while ((message = GT_GetIMsg(window->UserPort)) != NULL) {
            class = message->Class;
            code = message->Code;
            gadget_id = class == IDCMP_GADGETUP
                ? ((struct Gadget *)message->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(message);
            if (class == IDCMP_CLOSEWINDOW || gadget_id == GID_CANCEL) {
                running = FALSE;
            } else if (gadget_id == GID_RESULTS) {
                selected_index = (LONG)code;
            } else if (gadget_id == GID_SEARCH ||
                       (class == IDCMP_VANILLAKEY && code == 13)) {
                struct AmiWeatherResult result;
                const char *entered = (const char *)((struct StringInfo *)
                    string_gadget->SpecialInfo)->Buffer;
                if (!entered || !entered[0]) {
                    result.error = AMIWEATHER_ERR_ARGUMENT;
                    result.http_status = 0;
                    snprintf(result.diagnostic, sizeof(result.diagnostic),
                             "%s", "Enter a place name");
                    show_result_requester(window, "Location search", &result);
                    continue;
                }
                GT_SetGadgetAttrs(list_gadget, window, NULL,
                                  GTLV_Labels, (ULONG)~0, TAG_DONE);
                initialize_list(&labels);
                memset(&results, 0, sizeof(results));
                selected_index = -1;
                GT_SetGadgetAttrs(list_gadget, window, NULL,
                                  GTLV_Labels, (ULONG)&labels, TAG_DONE);
                SetWindowTitles(window, (UBYTE *)"Searching...", (UBYTE *)-1);
                result = amiweather_geocode_search(entered, &results);
                SetWindowTitles(window, (UBYTE *)"Location search", (UBYTE *)-1);
                if (result.error != AMIWEATHER_OK) {
                    show_result_requester(window, "Location search", &result);
                    continue;
                }
                GT_SetGadgetAttrs(list_gadget, window, NULL,
                                  GTLV_Labels, (ULONG)~0, TAG_DONE);
                initialize_list(&labels);
                for (i = 0; i < results.count; ++i) {
                    amiweather_location_format(&results.locations[i], display[i],
                                               sizeof(display[i]));
                    memset(&nodes[i], 0, sizeof(nodes[i]));
                    nodes[i].ln_Name = display[i];
                    AddTail(&labels, &nodes[i]);
                }
                selected_index = results.count ? 0 : -1;
                GT_SetGadgetAttrs(list_gadget, window, NULL,
                                  GTLV_Labels, (ULONG)&labels,
                                  GTLV_Selected, 0, TAG_DONE);
            } else if (gadget_id == GID_USE && selected_index >= 0 &&
                       (size_t)selected_index < results.count) {
                *selected = results.locations[selected_index];
                accepted = TRUE;
                running = FALSE;
            } else if (class == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(window);
                GT_EndRefresh(window, TRUE);
            }
        }
    }
cleanup:
    if (window) CloseWindow(window);
    if (gadgets) FreeGadgets(gadgets);
    return accepted;
}

static void ensure_preferences_directories(void)
{
    BPTR lock;
    lock = CreateDir((CONST_STRPTR)"ENVARC:AmiWeather");
    if (lock) UnLock(lock);
    lock = CreateDir((CONST_STRPTR)"ENV:AmiWeather");
    if (lock) UnLock(lock);
}

static enum AmiWeatherPreferencesError save_preferences(
    struct AmiWeatherPreferences *preferences)
{
    enum AmiWeatherPreferencesError result;
    ensure_preferences_directories();
    result = amiweather_preferences_save_file(AMIWEATHER_ENVARC_PATH, preferences);
    if (result != AMIWEATHER_PREFS_OK)
        return result;
    result = amiweather_preferences_save_file(AMIWEATHER_ENV_PATH, preferences);
    if (result != AMIWEATHER_PREFS_OK)
        DeleteFile((CONST_STRPTR)AMIWEATHER_ENV_PATH);
    return AMIWEATHER_PREFS_OK;
}

static enum AmiWeatherPreferencesError load_preferences(
    struct AmiWeatherPreferences *preferences)
{
    enum AmiWeatherPreferencesError env_result, archive_result;
    env_result = amiweather_preferences_load_file(AMIWEATHER_ENV_PATH, preferences);
    if (env_result == AMIWEATHER_PREFS_OK)
        return env_result;
    archive_result = amiweather_preferences_load_file(AMIWEATHER_ENVARC_PATH,
                                                       preferences);
    if (archive_result == AMIWEATHER_PREFS_OK)
        return AMIWEATHER_PREFS_OK;
    if (env_result == AMIWEATHER_PREFS_NOT_FOUND &&
        archive_result == AMIWEATHER_PREFS_NOT_FOUND)
        return AMIWEATHER_PREFS_NOT_FOUND;
    return archive_result;
}

static void show_preferences_error(struct Window *parent, const char *title,
                                   enum AmiWeatherPreferencesError error)
{
    struct EasyStruct request;
    request.es_StructSize = sizeof(request);
    request.es_Flags = 0;
    request.es_Title = (STRPTR)title;
    request.es_TextFormat = (STRPTR)amiweather_preferences_error_string(error);
    request.es_GadgetFormat = (STRPTR)"OK";
    EasyRequestArgs(parent, &request, NULL, NULL);
}

static BOOL preferences_window(struct Screen *screen, APTR visual_info,
                               struct AmiWeatherPreferences *candidate)
{
    static STRPTR temperature_labels[] = { (STRPTR)"Celsius", (STRPTR)"Fahrenheit", NULL };
    static STRPTR wind_labels[] = { (STRPTR)"km/h", (STRPTR)"mph", NULL };
    struct Window *window = NULL;
    struct Gadget *gadgets = NULL, *gadget;
    struct NewGadget ng;
    struct IntuiMessage *message;
    WORD font_height = screen->Font ? screen->Font->ta_YSize : 8;
    WORD width = screen->Width - 20 < 340 ? screen->Width - 20 : 340;
    WORD height = screen->Height - 20 < 145 ? screen->Height - 20 : 145;
    WORD top = screen->WBorTop + font_height + 10;
    WORD button_height = font_height + 8;
    BOOL running = TRUE, accepted = FALSE;
    ULONG temperature = (ULONG)candidate->temperature_unit;
    ULONG wind = (ULONG)candidate->wind_unit;

    gadget = CreateContext(&gadgets);
    if (!gadget) goto cleanup;
#define PREF_GADGET(left, y, w, h, text, id) do { memset(&ng, 0, sizeof(ng)); \
    ng.ng_LeftEdge=(left); ng.ng_TopEdge=(y); ng.ng_Width=(w); ng.ng_Height=(h); \
    ng.ng_GadgetText=(CONST_STRPTR)(text); ng.ng_GadgetID=(id); \
    ng.ng_VisualInfo=visual_info; ng.ng_TextAttr=screen->Font; } while (0)
    PREF_GADGET(105, top, width - 120, button_height, "Temperature:", GID_PREF_TEMPERATURE);
    gadget = CreateGadget(CYCLE_KIND, gadget, &ng, GTCY_Labels, (ULONG)temperature_labels,
                          GTCY_Active, temperature, TAG_DONE);
    if (!gadget) goto cleanup;
    PREF_GADGET(105, top + button_height + 8, width - 120, button_height,
                "Wind:", GID_PREF_WIND);
    gadget = CreateGadget(CYCLE_KIND, gadget, &ng, GTCY_Labels, (ULONG)wind_labels,
                          GTCY_Active, wind, TAG_DONE);
    if (!gadget) goto cleanup;
    PREF_GADGET(12, height - button_height - screen->WBorBottom - 5,
                80, button_height, "_Save", GID_PREF_SAVE);
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng, GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
    PREF_GADGET(100, height - button_height - screen->WBorBottom - 5,
                80, button_height, "_Cancel", GID_PREF_CANCEL);
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng, GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
#undef PREF_GADGET
    window = OpenWindowTags(NULL, WA_Title, (ULONG)"AmiWeather Preferences",
        WA_CustomScreen, (ULONG)screen, WA_Width, (ULONG)width, WA_Height, (ULONG)height,
        WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE,
        WA_Gadgets, (ULONG)gadgets,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW, TAG_DONE);
    if (!window) goto cleanup;
    GT_RefreshWindow(window, NULL);
    while (running) {
        Wait(1UL << window->UserPort->mp_SigBit);
        while ((message = GT_GetIMsg(window->UserPort)) != NULL) {
            ULONG class = message->Class;
            UWORD code = message->Code;
            UWORD id = class == IDCMP_GADGETUP
                ? ((struct Gadget *)message->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(message);
            if (class == IDCMP_CLOSEWINDOW || id == GID_PREF_CANCEL) running = FALSE;
            else if (id == GID_PREF_TEMPERATURE) temperature = code;
            else if (id == GID_PREF_WIND) wind = code;
            else if (id == GID_PREF_SAVE) {
                if (temperature <= AMIWEATHER_TEMPERATURE_FAHRENHEIT &&
                    wind <= AMIWEATHER_WIND_MPH) {
                    candidate->temperature_unit = (enum AmiWeatherTemperatureUnit)temperature;
                    candidate->wind_unit = (enum AmiWeatherWindUnit)wind;
                    accepted = TRUE; running = FALSE;
                }
            } else if (class == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(window); GT_EndRefresh(window, TRUE);
            }
        }
    }
cleanup:
    if (window) CloseWindow(window);
    if (gadgets) FreeGadgets(gadgets);
    return accepted;
}

static void refresh_saved_labels(struct Gadget *list_gadget, struct Window *window,
    struct List *labels, struct Node *nodes, char display[][AMIWEATHER_LOCATION_DISPLAY_SIZE],
    const struct AmiWeatherPreferences *preferences)
{
    size_t i;
    GT_SetGadgetAttrs(list_gadget, window, NULL, GTLV_Labels, (ULONG)~0, TAG_DONE);
    initialize_list(labels);
    for (i = 0; i < preferences->saved_location_count; ++i) {
        amiweather_location_format(&preferences->saved_locations[i], display[i],
                                   AMIWEATHER_LOCATION_DISPLAY_SIZE);
        if (preferences->has_default_location && preferences->default_location_index == i)
            strncat(display[i], " (default)",
                    AMIWEATHER_LOCATION_DISPLAY_SIZE - strlen(display[i]) - 1);
        memset(&nodes[i], 0, sizeof(nodes[i]));
        nodes[i].ln_Name = display[i]; AddTail(labels, &nodes[i]);
    }
    GT_SetGadgetAttrs(list_gadget, window, NULL, GTLV_Labels, (ULONG)labels,
                      preferences->saved_location_count ? GTLV_Selected : TAG_IGNORE,
                      0, TAG_DONE);
}

static BOOL saved_locations_window(struct Screen *screen, APTR visual_info,
    struct Window *parent, struct AmiWeatherPreferences *preferences,
    const struct AmiWeatherLocation *current, struct AmiWeatherLocation *selected)
{
    struct Window *window = NULL;
    struct Gadget *gadgets = NULL, *gadget, *list_gadget = NULL;
    struct NewGadget ng;
    struct List labels;
    struct Node nodes[AMIWEATHER_MAX_SAVED_LOCATIONS];
    char display[AMIWEATHER_MAX_SAVED_LOCATIONS][AMIWEATHER_LOCATION_DISPLAY_SIZE];
    struct IntuiMessage *message;
    WORD font_height = screen->Font ? screen->Font->ta_YSize : 8;
    WORD width = screen->Width - 20 < 500 ? screen->Width - 20 : 500;
    WORD height = screen->Height - 20 < 210 ? screen->Height - 20 : 210;
    WORD top = screen->WBorTop + font_height + 10;
    WORD button_height = font_height + 8;
    LONG selected_index = preferences->saved_location_count ? 0 : -1;
    BOOL running = TRUE, use_location = FALSE;
    initialize_list(&labels);
    gadget = CreateContext(&gadgets); if (!gadget) goto cleanup;
#define SAVED_GADGET(left,y,w,h,text,id) do { memset(&ng,0,sizeof(ng)); \
    ng.ng_LeftEdge=(left); ng.ng_TopEdge=(y); ng.ng_Width=(w); ng.ng_Height=(h); \
    ng.ng_GadgetText=(CONST_STRPTR)(text); ng.ng_GadgetID=(id); \
    ng.ng_VisualInfo=visual_info; ng.ng_TextAttr=screen->Font; } while(0)
    SAVED_GADGET(12, top, width - 24, height - top - button_height - 18,
                 NULL, GID_SAVED_LIST);
    gadget = CreateGadget(LISTVIEW_KIND, gadget, &ng, GTLV_Labels, (ULONG)&labels,
                          GTLV_ShowSelected, NULL, TAG_DONE);
    if (!gadget) goto cleanup;
    list_gadget = gadget;
    SAVED_GADGET(8, height-button_height-screen->WBorBottom-5, 64, button_height,
                 "_Use", GID_SAVED_USE);
    gadget=CreateGadget(BUTTON_KIND,gadget,&ng,GT_Underscore,(ULONG)'_',TAG_DONE); if(!gadget)goto cleanup;
    SAVED_GADGET(76, height-button_height-screen->WBorBottom-5, 92, button_height,
                 "_Add Current", GID_SAVED_ADD);
    gadget=CreateGadget(BUTTON_KIND,gadget,&ng,GT_Underscore,(ULONG)'_',TAG_DONE); if(!gadget)goto cleanup;
    SAVED_GADGET(172, height-button_height-screen->WBorBottom-5, 72, button_height,
                 "_Remove", GID_SAVED_REMOVE);
    gadget=CreateGadget(BUTTON_KIND,gadget,&ng,GT_Underscore,(ULONG)'_',TAG_DONE); if(!gadget)goto cleanup;
    SAVED_GADGET(248, height-button_height-screen->WBorBottom-5, 72, button_height,
                 "_Default", GID_SAVED_DEFAULT);
    gadget=CreateGadget(BUTTON_KIND,gadget,&ng,GT_Underscore,(ULONG)'_',TAG_DONE); if(!gadget)goto cleanup;
    SAVED_GADGET(324, height-button_height-screen->WBorBottom-5, 64, button_height,
                 "_Close", GID_SAVED_CLOSE);
    gadget=CreateGadget(BUTTON_KIND,gadget,&ng,GT_Underscore,(ULONG)'_',TAG_DONE); if(!gadget)goto cleanup;
#undef SAVED_GADGET
    window=OpenWindowTags(NULL,WA_Title,(ULONG)"Saved locations",WA_CustomScreen,(ULONG)screen,
        WA_Width,(ULONG)width,WA_Height,(ULONG)height,WA_DragBar,TRUE,WA_DepthGadget,TRUE,
        WA_CloseGadget,TRUE,WA_Activate,TRUE,WA_Gadgets,(ULONG)gadgets,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW,TAG_DONE);
    if(!window)goto cleanup;
    refresh_saved_labels(list_gadget,window,&labels,nodes,display,preferences);
    GT_RefreshWindow(window,NULL);
    while(running){
        Wait(1UL<<window->UserPort->mp_SigBit);
        while((message=GT_GetIMsg(window->UserPort))!=NULL){
            ULONG class=message->Class; UWORD code=message->Code;
            UWORD id=class==IDCMP_GADGETUP?((struct Gadget*)message->IAddress)->GadgetID:0;
            GT_ReplyIMsg(message);
            if(class==IDCMP_CLOSEWINDOW||id==GID_SAVED_CLOSE)running=FALSE;
            else if(id==GID_SAVED_LIST)selected_index=(LONG)code;
            else if(id==GID_SAVED_USE&&selected_index>=0&&
                    (size_t)selected_index<preferences->saved_location_count){
                *selected=preferences->saved_locations[selected_index]; use_location=TRUE; running=FALSE;
            } else if(id==GID_SAVED_ADD){
                struct AmiWeatherPreferences changed=*preferences;
                enum AmiWeatherPreferencesError error=amiweather_preferences_add_location(&changed,current);
                if(error==AMIWEATHER_PREFS_OK)error=save_preferences(&changed);
                if(error==AMIWEATHER_PREFS_OK){*preferences=changed;selected_index=(LONG)preferences->saved_location_count-1;
                    refresh_saved_labels(list_gadget,window,&labels,nodes,display,preferences);}
                else show_preferences_error(parent,"Save location",error);
            } else if(id==GID_SAVED_REMOVE&&selected_index>=0&&
                      (size_t)selected_index<preferences->saved_location_count){
                struct AmiWeatherPreferences changed=*preferences;
                enum AmiWeatherPreferencesError error=amiweather_preferences_remove_location(&changed,(size_t)selected_index);
                if(error==AMIWEATHER_PREFS_OK)error=save_preferences(&changed);
                if(error==AMIWEATHER_PREFS_OK){*preferences=changed;
                    if((size_t)selected_index>=preferences->saved_location_count)--selected_index;
                    refresh_saved_labels(list_gadget,window,&labels,nodes,display,preferences);}
                else show_preferences_error(parent,"Remove location",error);
            } else if(id==GID_SAVED_DEFAULT&&selected_index>=0&&
                      (size_t)selected_index<preferences->saved_location_count){
                struct AmiWeatherPreferences changed=*preferences;
                enum AmiWeatherPreferencesError error;
                amiweather_preferences_set_default(&changed,(size_t)selected_index);
                error=save_preferences(&changed);
                if(error==AMIWEATHER_PREFS_OK){*preferences=changed;
                    refresh_saved_labels(list_gadget,window,&labels,nodes,display,preferences);}
                else show_preferences_error(parent,"Default location",error);
            } else if(class==IDCMP_REFRESHWINDOW){GT_BeginRefresh(window);GT_EndRefresh(window,TRUE);}
        }
    }
cleanup:
    if (window) CloseWindow(window);
    if (gadgets) FreeGadgets(gadgets);
    return use_location;
}

static void hourly_window(struct Screen *screen, APTR visual_info,
                          struct Window *parent,
                          const struct ApplicationState *application)
{
    struct Window *window = NULL;
    struct Gadget *gadgets = NULL, *gadget;
    struct NewGadget ng;
    struct List labels;
    struct Node nodes[AMIWEATHER_HOURLY_COUNT];
    char display[AMIWEATHER_HOURLY_COUNT][96];
    char location_name[AMIWEATHER_LOCATION_DISPLAY_SIZE];
    char title[AMIWEATHER_LOCATION_DISPLAY_SIZE + 32];
    struct IntuiMessage *message;
    WORD font_height = screen->Font ? screen->Font->ta_YSize : 8;
    WORD width = screen->Width - 20 < 470 ? screen->Width - 20 : 470;
    WORD height = screen->Height - 20 < 236 ? screen->Height - 20 : 236;
    WORD top = screen->WBorTop + font_height + 8;
    WORD button_height = font_height + 8;
    BOOL running = TRUE;
    size_t i;

    if (!application->has_report || application->report.hourly.count == 0) {
        struct AmiWeatherResult result;
        result.error = AMIWEATHER_ERR_NOT_FOUND;
        result.http_status = 0;
        snprintf(result.diagnostic, sizeof(result.diagnostic),
                 "%s", "No hourly forecast is available");
        show_result_requester(parent, "Hourly forecast", &result);
        return;
    }
    initialize_list(&labels);
    for (i = 0; i < application->report.hourly.count; ++i) {
        const struct AmiWeatherHourly *entry = &application->report.hourly.entries[i];
        char time[AMIWEATHER_COMPACT_TIME_SIZE];
        char temperature[24];
        amiweather_format_hourly_time(entry->time, time, sizeof(time));
        format_tenths(temperature, sizeof(temperature), entry->temperature);
        if (entry->has_precipitation_probability)
            snprintf(display[i], sizeof(display[i]),
                     "%-5s  %-18s  %s \260%s  %3d%%",
                     time, amiweather_wmo_compact_description(entry->weather_code),
                     temperature,
                     amiweather_temperature_unit_label(application->preferences.temperature_unit),
                     entry->precipitation_probability);
        else
            snprintf(display[i], sizeof(display[i]),
                     "%-5s  %-18s  %s \260%s",
                     time, amiweather_wmo_compact_description(entry->weather_code),
                     temperature,
                     amiweather_temperature_unit_label(application->preferences.temperature_unit));
        memset(&nodes[i], 0, sizeof(nodes[i]));
        nodes[i].ln_Name = display[i];
        AddTail(&labels, &nodes[i]);
    }
    amiweather_location_format(&application->location, location_name,
                               sizeof(location_name));
    snprintf(title, sizeof(title), "Hourly Forecast - %s", location_name);
    gadget = CreateContext(&gadgets);
    if (!gadget) goto cleanup;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = 12; ng.ng_TopEdge = top;
    ng.ng_Width = width - 24;
    ng.ng_Height = height - top - button_height - screen->WBorBottom - 12;
    ng.ng_GadgetID = GID_RESULTS; ng.ng_VisualInfo = visual_info;
    ng.ng_TextAttr = screen->Font;
    gadget = CreateGadget(LISTVIEW_KIND, gadget, &ng,
                          GTLV_Labels, (ULONG)&labels, TAG_DONE);
    if (!gadget) goto cleanup;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = width - 92;
    ng.ng_TopEdge = height - button_height - screen->WBorBottom - 5;
    ng.ng_Width = 80; ng.ng_Height = button_height;
    ng.ng_GadgetText = (CONST_STRPTR)"_Close";
    ng.ng_GadgetID = GID_SAVED_CLOSE; ng.ng_VisualInfo = visual_info;
    ng.ng_TextAttr = screen->Font;
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
    window = OpenWindowTags(NULL, WA_Title, (ULONG)title,
        WA_CustomScreen, (ULONG)screen, WA_Width, (ULONG)width,
        WA_Height, (ULONG)height, WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_Gadgets, (ULONG)gadgets,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,
        TAG_DONE);
    if (!window) goto cleanup;
    GT_RefreshWindow(window, NULL);
    while (running) {
        Wait(1UL << window->UserPort->mp_SigBit);
        while ((message = GT_GetIMsg(window->UserPort)) != NULL) {
            ULONG class = message->Class;
            UWORD id = class == IDCMP_GADGETUP
                ? ((struct Gadget *)message->IAddress)->GadgetID : 0;
            GT_ReplyIMsg(message);
            if (class == IDCMP_CLOSEWINDOW || id == GID_SAVED_CLOSE)
                running = FALSE;
            else if (class == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(window);
                GT_EndRefresh(window, TRUE);
            }
        }
    }
cleanup:
    if (window) CloseWindow(window);
    if (gadgets) FreeGadgets(gadgets);
}

int amiweather_gui_run(void)
{
    struct Screen *screen = NULL;
    struct Window *window = NULL;
    struct Gadget *gadgets = NULL;
    struct Gadget *gadget;
    struct NewGadget ng;
    struct WindowLayout layout;
    struct ApplicationState application;
    APTR visual_info = NULL;
    struct IntuiMessage *message;
    ULONG class;
    UWORD code;
    BOOL running = TRUE;
    BOOL refresh_requested;
    BOOL change_requested;
    BOOL locations_requested;
    BOOL preferences_requested;
    BOOL hourly_requested;
    struct Menu *menus = NULL;
    enum MenuAction menu_action;
    enum AmiWeatherPreferencesError preferences_load_result;
    char window_title[32];
    int result = 20;

    memset(&application, 0, sizeof(application));
    application.view = VIEW_LOADING;
    amiweather_preferences_default(&application.preferences);
    preferences_load_result = load_preferences(&application.preferences);
    if (preferences_load_result != AMIWEATHER_PREFS_OK &&
        preferences_load_result != AMIWEATHER_PREFS_NOT_FOUND)
        amiweather_preferences_default(&application.preferences);
    if (application.preferences.has_default_location)
        application.location = application.preferences.saved_locations[
            application.preferences.default_location_index];
    else
        amiweather_location_default(&application.location);
    snprintf(window_title, sizeof(window_title), "AmiWeather %s", AMIWEATHER_VERSION);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39);
    GadToolsBase = OpenLibrary("gadtools.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39);
    if (!IntuitionBase || !GadToolsBase || !GfxBase)
        goto cleanup;
    screen = LockPubScreen(NULL);
    if (!screen)
        goto cleanup;
    visual_info = GetVisualInfo(screen, TAG_DONE);
    if (!visual_info)
        goto cleanup;
    calculate_layout(screen, &layout);
    menus = create_application_menus(visual_info);
    if (!menus)
        goto cleanup;

    gadget = CreateContext(&gadgets);
    if (!gadget)
        goto cleanup;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = layout.gadget_left;
    ng.ng_TopEdge = layout.gadget_top;
    ng.ng_Width = layout.gadget_width;
    ng.ng_Height = layout.gadget_height;
    ng.ng_GadgetText = (CONST_STRPTR)"_Refresh";
    ng.ng_GadgetID = GID_REFRESH;
    ng.ng_VisualInfo = visual_info;
    ng.ng_TextAttr = screen->Font;
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget)
        goto cleanup;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = layout.hourly_left; ng.ng_TopEdge = layout.gadget_top;
    ng.ng_Width = layout.gadget_width; ng.ng_Height = layout.gadget_height;
    ng.ng_GadgetText = (CONST_STRPTR)"_Hourly..."; ng.ng_GadgetID = GID_HOURLY;
    ng.ng_VisualInfo = visual_info; ng.ng_TextAttr = screen->Font;
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget) goto cleanup;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = layout.change_left;
    ng.ng_TopEdge = layout.gadget_top;
    ng.ng_Width = layout.gadget_width;
    ng.ng_Height = layout.gadget_height;
    ng.ng_GadgetText = (CONST_STRPTR)"_Change...";
    ng.ng_GadgetID = GID_CHANGE;
    ng.ng_VisualInfo = visual_info;
    ng.ng_TextAttr = screen->Font;
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng,
                          GT_Underscore, (ULONG)'_', TAG_DONE);
    if (!gadget)
        goto cleanup;

    window = OpenWindowTags(NULL,
        WA_Title, (ULONG)window_title,
        WA_CustomScreen, (ULONG)screen,
        WA_Width, (ULONG)layout.width,
        WA_Height, (ULONG)layout.height,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_Gadgets, (ULONG)gadgets,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP |
                  IDCMP_REFRESHWINDOW | IDCMP_VANILLAKEY | IDCMP_MENUPICK,
        TAG_DONE);
    if (!window)
        goto cleanup;
    SetMenuStrip(window, menus);

    layout.left = window->BorderLeft + 12;
    layout.content_top = window->BorderTop + window->RPort->TxHeight + 6;
    GT_RefreshWindow(window, NULL);
    draw_content(window, &layout, &application);
    fetch_weather(window, &layout, &application);
    if (preferences_load_result != AMIWEATHER_PREFS_OK &&
        preferences_load_result != AMIWEATHER_PREFS_NOT_FOUND)
        show_preferences_error(window, "Preferences ignored", preferences_load_result);

    while (running) {
        Wait(1UL << window->UserPort->mp_SigBit);
        while ((message = GT_GetIMsg(window->UserPort)) != NULL) {
            class = message->Class;
            code = message->Code;
            refresh_requested = FALSE;
            change_requested = FALSE;
            locations_requested = FALSE;
            preferences_requested = FALSE;
            hourly_requested = FALSE;
            menu_action = 0;
            if (class == IDCMP_CLOSEWINDOW)
                running = FALSE;
            else if (class == IDCMP_MENUPICK) {
                struct MenuItem *item = ItemAddress(menus, code);
                if (item)
                    menu_action = (enum MenuAction)(ULONG)GTMENUITEM_USERDATA(item);
                if (menu_action == MENU_QUIT)
                    running = FALSE;
                else if (menu_action == MENU_REFRESH)
                    refresh_requested = TRUE;
                else if (menu_action == MENU_ABOUT)
                    show_about(window);
                else if (menu_action == MENU_CHANGE)
                    change_requested = TRUE;
                else if (menu_action == MENU_LOCATIONS)
                    locations_requested = TRUE;
                else if (menu_action == MENU_HOURLY)
                    hourly_requested = TRUE;
                else if (menu_action == MENU_PREFERENCES)
                    preferences_requested = TRUE;
            }
            else if (class == IDCMP_GADGETUP &&
                     ((struct Gadget *)message->IAddress)->GadgetID == GID_REFRESH)
                refresh_requested = TRUE;
            else if (class == IDCMP_GADGETUP &&
                     ((struct Gadget *)message->IAddress)->GadgetID == GID_CHANGE)
                change_requested = TRUE;
            else if (class == IDCMP_GADGETUP &&
                     ((struct Gadget *)message->IAddress)->GadgetID == GID_LOCATIONS)
                locations_requested = TRUE;
            else if (class == IDCMP_GADGETUP &&
                     ((struct Gadget *)message->IAddress)->GadgetID == GID_PREFERENCES)
                preferences_requested = TRUE;
            else if (class == IDCMP_GADGETUP &&
                     ((struct Gadget *)message->IAddress)->GadgetID == GID_HOURLY)
                hourly_requested = TRUE;
            else if (class == IDCMP_VANILLAKEY && (code == 'r' || code == 'R'))
                refresh_requested = TRUE;
            else if (class == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(window);
                draw_content(window, &layout, &application);
                GT_EndRefresh(window, TRUE);
            }
            GT_ReplyIMsg(message);
            if (refresh_requested && running)
                fetch_weather(window, &layout, &application);
            if (change_requested && running) {
                struct AmiWeatherLocation candidate;
                if (search_location(screen, visual_info, &candidate)) {
                    struct AmiWeatherReport updated;
                    struct AmiWeatherResult fetch_result;
                    application.view = VIEW_LOADING;
                    draw_content(window, &layout, &application);
                    fetch_result = amiweather_openmeteo_fetch_report_units(
                        &candidate, application.preferences.temperature_unit,
                        application.preferences.wind_unit, &updated);
                    if (fetch_result.error == AMIWEATHER_OK) {
                        application.location = candidate;
                        application.report = updated;
                        application.has_report = TRUE;
                        application.result = fetch_result;
                        application.view = VIEW_WEATHER;
                    } else {
                        application.view = VIEW_WEATHER;
                        show_result_requester(window, "Weather update", &fetch_result);
                    }
                    draw_content(window, &layout, &application);
                }
            }
            if (locations_requested && running) {
                struct AmiWeatherLocation candidate;
                if (saved_locations_window(screen, visual_info, window,
                        &application.preferences, &application.location, &candidate)) {
                    struct AmiWeatherReport updated;
                    struct AmiWeatherResult fetch_result;
                    application.view = VIEW_LOADING;
                    draw_content(window, &layout, &application);
                    fetch_result = amiweather_openmeteo_fetch_report_units(
                        &candidate, application.preferences.temperature_unit,
                        application.preferences.wind_unit, &updated);
                    if (fetch_result.error == AMIWEATHER_OK) {
                        application.location = candidate;
                        application.report = updated;
                        application.result = fetch_result;
                        application.has_report = TRUE;
                        application.view = VIEW_WEATHER;
                    } else {
                        application.view = application.has_report ? VIEW_WEATHER : VIEW_ERROR;
                        show_result_requester(window, "Weather update", &fetch_result);
                    }
                    draw_content(window, &layout, &application);
                }
            }
            if (preferences_requested && running) {
                struct AmiWeatherPreferences candidate_preferences = application.preferences;
                if (preferences_window(screen, visual_info, &candidate_preferences)) {
                    struct AmiWeatherReport updated;
                    struct AmiWeatherResult fetch_result;
                    enum AmiWeatherPreferencesError save_result;
                    application.view = VIEW_LOADING;
                    draw_content(window, &layout, &application);
                    fetch_result = amiweather_openmeteo_fetch_report_units(
                        &application.location, candidate_preferences.temperature_unit,
                        candidate_preferences.wind_unit, &updated);
                    if (fetch_result.error != AMIWEATHER_OK) {
                        application.view = application.has_report ? VIEW_WEATHER : VIEW_ERROR;
                        show_result_requester(window, "Unit update", &fetch_result);
                    } else {
                        save_result = save_preferences(&candidate_preferences);
                        if (save_result != AMIWEATHER_PREFS_OK) {
                            application.view = application.has_report ? VIEW_WEATHER : VIEW_ERROR;
                            show_preferences_error(window, "Preferences not saved", save_result);
                        } else {
                            application.preferences = candidate_preferences;
                            application.report = updated;
                            application.result = fetch_result;
                            application.has_report = TRUE;
                            application.view = VIEW_WEATHER;
                        }
                    }
                    draw_content(window, &layout, &application);
                }
            }
            if (hourly_requested && running)
                hourly_window(screen, visual_info, window, &application);
        }
    }
    result = 0;

cleanup:
    if (window && menus)
        ClearMenuStrip(window);
    if (window)
        CloseWindow(window);
    if (menus)
        FreeMenus(menus);
    if (gadgets)
        FreeGadgets(gadgets);
    if (visual_info)
        FreeVisualInfo(visual_info);
    if (screen)
        UnlockPubScreen(NULL, screen);
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
    if (GadToolsBase)
        CloseLibrary(GadToolsBase);
    if (IntuitionBase)
        CloseLibrary((struct Library *)IntuitionBase);
    return result;
}
