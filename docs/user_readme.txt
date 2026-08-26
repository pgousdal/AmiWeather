AmiWeather 0.1.0
================

AmiWeather is a native weather client for AmigaOS 3.x (68020 or newer). It
shows current conditions, a five-day daily forecast, and the next twelve local
forecast hours using Open-Meteo.

Installation
------------
Copy AmiWeather to any drawer and run it from Workbench or Shell. It uses the
public screen and standard Intuition/GadTools controls; no icon or tooltypes
are required. AmiWeather provisions its own runtime stack; no manual Shell
Stack command is required, including on AmigaOS 3.1.

The Amiga build requires a working bsdsocket.library-compatible TCP/IP stack,
DNS access, and AmiSSL 5 for HTTPS. AmiTCP_NG is tested but not mandatory.

Locations and preferences
-------------------------
Change... searches for a place. The Location menu can save up to eight places,
use a saved place without geocoding, remove entries, and choose the startup
default. Settings > Preferences... selects Celsius/Fahrenheit and km/h/mph.
Project > About... shows program and license information; Project > Quit exits.
Changes are fetched and saved before being applied.

Configuration is stored as versioned text in ENV:AmiWeather/preferences and
ENVARC:AmiWeather/preferences. ENV: is preferred at startup; ENVARC: is the
persistent fallback. Missing files are normal first-run state.

Weather views
-------------
Refresh updates current, daily, and hourly data synchronously. The main window
shows five daily rows. Forecast > Daily Forecast..., 7 Day Forecast..., and
14 Day Forecast... open bounded native lists; Hourly... shows twelve local
hours; Weather Details... shows available current measurements. Opening any
view uses the last successful report and makes no network request. Network
requests can temporarily block the interface until AmiAPI's finite timeout
expires.

Troubleshooting
---------------
If weather cannot be fetched, verify DNS, the bsdsocket-compatible TCP/IP
stack, and AmiSSL 5. A failed refresh keeps the last successful display and
does not erase preferences.

Credits
-------
Weather/geocoding: Open-Meteo. HTTP/API SDK: AmiAPI. HTTPS/TLS: AmiSSL 5.
These projects are independent; no endorsement is implied.
