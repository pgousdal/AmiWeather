AmiWeatherDoor 0.1.0
====================

AmiWeatherDoor is the plain-ASCII terminal/BBS frontend of AmiWeather.
ABBS 3.3 is the first and currently only runtime-qualified BBS.

Commands
--------
H Hourly       D Daily        7 7 Day       A 14 Day
W Details      L Location     R Refresh     Q Return to BBS

Location search accepts up to 63 characters. Choose a numbered result to
fetch and commit a new report; 0 or an empty initial search cancels silently.
Failed searches or weather requests preserve the current report.

Installation on ABBS 3.3
-------------------------
Copy AmiWeatherDoor and AmiWeather.script to ABBS:Doors/AmiWeather/.
Add this Shell-door entry to Node0Config or the appropriate NodeXConfig:

S ABBS:Doors/AmiWeather/AmiWeather.script ; AmiWeather

Add matching menu text from Node0Menu.example. The OPEN number is determined
by the position of valid door entries. Use OPEN n from ABBS.

Keep failat 2147483647 before the executable and EndCLI after it. ABBS Shell
doors can expose an interactive AmigaDOS Shell if a script aborts before
EndCLI; this fail-closed launcher behavior was manually qualified.

Requirements
------------
AmigaOS 3.1 or later, 68020 or later, a working bsdsocket.library-compatible
TCP/IP stack, DNS, Internet access, and AmiSSL 5. No FPU or MMU is required,
and no manual Stack command is needed: the door provisions a private 32 KiB
runtime stack. Plain ASCII is the baseline; ANSI is not required.

Weather data and geocoding use Open-Meteo, through the external AmiAPI HTTP
SDK and AmiSSL 5 for HTTPS. Preferences use the existing AmiWeather
ENV:AmiWeather/preferences and ENVARC:AmiWeather/preferences policy; there
are no per-BBS-user preferences. AmiExpress, Paragon, and ARexx doors are not
supported.

License: MIT
Author: Per Gustav Ousdal
Contact: amiweather@ousdal.org
