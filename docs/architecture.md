# Architecture

```text
Intuition + GadTools UI
          |
    AmiWeather domain
          |
   Open-Meteo adapter
          |
        AmiAPI
          |
 bsdsocket + AmiSSL 5
```

The GUI consumes `AmiWeatherCurrent`, an application-owned value structure,
and application-level result codes. Open-Meteo field names and AmiAPI JSON
objects stay in the adapter. AmiAPI is an external SDK: AmiWeather includes
only `include/amiapi/*` public headers and links `libamiapi.a`; it neither
compiles AmiAPI sources nor accesses opaque implementation structures.

The adapter builds an HTTPS URL with AmiAPI's query builder, bounds the
response to 64 KiB, applies a 20-second network timeout, checks for a 2xx HTTP
status, and parses the response with AmiAPI's public JSON API. All acquired
network objects unwind through one cleanup path.

M1 models loading, valid-weather, and error presentation explicitly. The GUI
stores only AmiWeather domain values and application-level results. Launch and
explicit Refresh actions call the synchronous adapter; repainting reads the
stored state and never performs network I/O. A request can therefore block GUI
interaction until completion or timeout in M1. Background execution remains a
later design question rather than premature infrastructure.

## Native UI direction

AmiWeather should look native to the user's Amiga environment instead of
imposing a theme. It uses Intuition conventions, GadTools gadgets, system
fonts and pens, standard window borders, and the user-selected Workbench or
public screen. Layouts must remain usable at low resolutions and must not
assume square pixels, a fixed palette, a fixed font, RTG, or true colour.
Standard menus and keyboard behaviour should follow normal Amiga conventions.

Restrained custom drawing is appropriate for future weather symbols or
forecast visualization, but it must not replace standard gadgets merely for
appearance. The goal is a polished classic AmigaOS 3.x application, neither a
developer-only shell nor a modern interface transplanted onto AmigaOS.

The native application locks the default public screen, derives its layout from
the screen bounds, borders, and font metrics, obtains GadTools VisualInfo,
creates a standard Refresh button and window, processes IDCMP messages, and
releases the window, gadgets,
VisualInfo, screen lock, and libraries in reverse order. Large response data is
heap-owned by AmiAPI; AmiWeather has no large automatic buffers or recursive
application architecture.
