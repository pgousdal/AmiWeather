#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <graphics/gfxbase.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>

#include <string.h>

struct IntuitionBase *IntuitionBase;
struct Library *GadToolsBase;
struct GfxBase *GfxBase;

static void draw_content(struct Window *window)
{
    struct RastPort *rp = window->RPort;
    WORD left = window->BorderLeft + 12;
    WORD top = window->BorderTop + rp->TxHeight + 10;

    SetAPen(rp, window->WScreen->DetailPen);
    Move(rp, left, top);
    Text(rp, "AmiWeather", 10);
    Move(rp, left, top + rp->TxHeight + 8);
    Text(rp, "Native weather client for AmigaOS", 33);
    Move(rp, left, top + (rp->TxHeight + 8) * 2);
    Text(rp, "M0 foundation", 13);
}

int amiweather_gui_run(void)
{
    struct Screen *screen = NULL;
    struct Window *window = NULL;
    struct Gadget *gadgets = NULL;
    struct Gadget *gadget;
    struct NewGadget ng;
    APTR visual_info = NULL;
    struct IntuiMessage *message;
    ULONG class;
    BOOL running = TRUE;
    WORD width;
    WORD height;
    int result = 20;

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

    width = screen->Width > 410 ? 400 : screen->Width - 12;
    height = screen->Height > 180 ? 150 : screen->Height - 12;
    gadget = CreateContext(&gadgets);
    if (!gadget)
        goto cleanup;
    memset(&ng, 0, sizeof(ng));
    ng.ng_LeftEdge = width - 90;
    ng.ng_TopEdge = height - screen->WBorBottom - 28;
    ng.ng_Width = 72;
    ng.ng_Height = 18;
    ng.ng_GadgetText = (CONST_STRPTR)"Close";
    ng.ng_GadgetID = 1;
    ng.ng_VisualInfo = visual_info;
    ng.ng_TextAttr = screen->Font;
    gadget = CreateGadget(BUTTON_KIND, gadget, &ng, TAG_DONE);
    if (!gadget)
        goto cleanup;

    window = OpenWindowTags(NULL,
        WA_Title, (ULONG)"AmiWeather",
        WA_CustomScreen, (ULONG)screen,
        WA_Width, (ULONG)width,
        WA_Height, (ULONG)height,
        WA_MinWidth, 300UL,
        WA_MinHeight, 120UL,
        WA_MaxWidth, (ULONG)screen->Width,
        WA_MaxHeight, (ULONG)screen->Height,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_Activate, TRUE,
        WA_Gadgets, (ULONG)gadgets,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_REFRESHWINDOW,
        TAG_DONE);
    if (!window)
        goto cleanup;

    GT_RefreshWindow(window, NULL);
    draw_content(window);
    while (running) {
        Wait(1UL << window->UserPort->mp_SigBit);
        while ((message = GT_GetIMsg(window->UserPort)) != NULL) {
            class = message->Class;
            if (class == IDCMP_CLOSEWINDOW ||
                (class == IDCMP_GADGETUP &&
                 ((struct Gadget *)message->IAddress)->GadgetID == 1))
                running = FALSE;
            else if (class == IDCMP_REFRESHWINDOW) {
                GT_BeginRefresh(window);
                draw_content(window);
                GT_EndRefresh(window, TRUE);
            }
            GT_ReplyIMsg(message);
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
