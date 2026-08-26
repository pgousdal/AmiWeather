#include <amiweather/openmeteo.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <string.h>

#define GID_REFRESH 1

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
    struct AmiWeatherCurrent weather;
    struct AmiWeatherResult result;
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
};

static void calculate_layout(const struct Screen *screen,
                             struct WindowLayout *layout)
{
    WORD font_height = screen->Font ? screen->Font->ta_YSize : 8;
    WORD available_width = screen->Width - 12;
    WORD available_height = screen->Height - 12;
    WORD desired_height;

    layout->width = available_width < 400 ? available_width : 400;
    layout->left = screen->WBorLeft + 12;
    layout->content_top = screen->WBorTop + font_height + 10;
    layout->baseline_step = font_height + 4;
    layout->gadget_width = 88;
    layout->gadget_height = font_height + 10;
    desired_height = layout->content_top + 9 * layout->baseline_step +
                     layout->gadget_height + screen->WBorBottom + 10;
    layout->height = desired_height < available_height
        ? desired_height : available_height;
    layout->gadget_left = layout->left;
    layout->gadget_top = layout->height - screen->WBorBottom -
                         layout->gadget_height - 6;
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
    draw_line(rp, layout, 0, "Oslo");
    SetSoftStyle(rp, FS_NORMAL, supported_style);

    if (application->view == VIEW_LOADING) {
        draw_line(rp, layout, 2, "Fetching weather...");
        return;
    }
    if (application->view == VIEW_ERROR) {
        draw_line(rp, layout, 2, "Unable to update weather");
        draw_line(rp, layout, 4, visible_error_detail(&application->result));
        return;
    }

    {
        char value[32];

        format_tenths(
            value,
            sizeof(value),
            application->weather.temperature_c
        );
        snprintf(line, sizeof(line), "%s \260C", value);
    }

    SetSoftStyle(rp, FSF_BOLD, supported_style);
    draw_line(rp, layout, 2, line);
    SetSoftStyle(rp, FS_NORMAL, supported_style);
    draw_line(rp, layout, 3,
              amiweather_wmo_description(application->weather.weather_code));

    {
        char value[32];

        format_tenths(
            value,
            sizeof(value),
            application->weather.apparent_temperature_c
        );
        snprintf(line, sizeof(line), "Feels like %s \260C", value);
    }
    draw_line(rp, layout, 5, line);

    snprintf(
        line,
        sizeof(line),
        "Humidity %ld%%",
        (long)(application->weather.relative_humidity_percent + 0.5)
    );
    draw_line(rp, layout, 6, line);

    {
        char value[32];

        format_tenths(
            value,
            sizeof(value),
            application->weather.wind_speed_kmh
        );
        snprintf(line, sizeof(line), "Wind %s km/h", value);
    }
    draw_line(rp, layout, 7, line);
    snprintf(line, sizeof(line), "Observed %s UTC",
             application->weather.observation_time);
    draw_line(rp, layout, 8, line);
}

static void fetch_weather(struct Window *window,
                          const struct WindowLayout *layout,
                          struct ApplicationState *application)
{
    struct AmiWeatherCurrent updated;
    struct AmiWeatherResult result;

    application->view = VIEW_LOADING;
    draw_content(window, layout, application);
    result = amiweather_openmeteo_fetch_reference(&updated);
    application->result = result;
    if (result.error == AMIWEATHER_OK) {
        application->weather = updated;
        application->view = VIEW_WEATHER;
    } else {
        application->view = VIEW_ERROR;
    }
    draw_content(window, layout, application);
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
    int result = 20;

    memset(&application, 0, sizeof(application));
    application.view = VIEW_LOADING;
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

    window = OpenWindowTags(NULL,
        WA_Title, (ULONG)"AmiWeather",
        WA_CustomScreen, (ULONG)screen,
        WA_Width, (ULONG)layout.width,
        WA_Height, (ULONG)layout.height,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate, TRUE,
        WA_Gadgets, (ULONG)gadgets,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP |
                  IDCMP_REFRESHWINDOW | IDCMP_VANILLAKEY,
        TAG_DONE);
    if (!window)
        goto cleanup;

    layout.left = window->BorderLeft + 12;
    layout.content_top = window->BorderTop + window->RPort->TxHeight + 6;
    GT_RefreshWindow(window, NULL);
    draw_content(window, &layout, &application);
    fetch_weather(window, &layout, &application);

    while (running) {
        Wait(1UL << window->UserPort->mp_SigBit);
        while ((message = GT_GetIMsg(window->UserPort)) != NULL) {
            class = message->Class;
            code = message->Code;
            refresh_requested = FALSE;
            if (class == IDCMP_CLOSEWINDOW)
                running = FALSE;
            else if (class == IDCMP_GADGETUP &&
                     ((struct Gadget *)message->IAddress)->GadgetID == GID_REFRESH)
                refresh_requested = TRUE;
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
        }
    }
    result = 0;

cleanup:
    if (window)
        CloseWindow(window);
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
