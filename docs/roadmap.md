# Roadmap

- **M0 — Foundation (complete):** external AmiAPI integration, native GUI shell,
  deterministic Open-Meteo parsing, and a real-request proof.
- **M1 — Current conditions (complete):** fixed-Oslo current conditions,
  human-readable WMO descriptions, Refresh, and visible loading/error states in
  the native interface.
- **M2 — Location search and selection (complete):** bounded Open-Meteo
  geocoding, native GadTools search/result selection, and in-memory location
  state. Search and fetch remain synchronous; persistence is deferred.
- **M3 — Daily Forecast Presentation (complete):** bounded five-day daily
  forecast, shared current-plus-daily request, defensive parallel-array parsing,
  and a restrained classic Amiga forecast table.
- **M4 — Preferences and Saved Locations (complete):** versioned ENV:/ENVARC:
  persistence, eight bounded saved locations, startup default selection, and
  Celsius/Fahrenheit plus km/h/mph preferences.
- **M5 — Hourly Forecast (complete):** twelve bounded next-hour entries in the
  atomic current/daily report and a compact scrollable native hourly window.
- **M6 — Visual Polish, Weather Icons, and Aminet Packaging (complete):**
  restrained compiled-in icons, improved hierarchy, version 0.1.0, MIT license,
  and reproducible LHA packaging with executable/archive checksums.
- **M6.1 — Native Menus and About (complete):** native Project, Location,
  Forecast, and Settings menus, a compact About requester, and a reduced
  three-button primary action row.
- **M7 — Release Hardening / Aminet 0.1.0 (complete):** extended forecast
  views, native stack compatibility, and manual QA on Kickstart 40.68 /
  Workbench 40.42.
- **M8.0 — BBS Door Foundation (complete):** separate AmiWeatherDoor target,
  shared weather core, bounded ASCII terminal presentation, and a documented
  adapter boundary without guessed BBS ABI details.
- **M8.1 — ABBS 3.3 Integration (complete):** documented Shell-door launcher,
  bounded FullName/NodeNr context, fixed example configuration, and EndCLI
  cleanup. Runtime qualification remains deferred.
- **M8.2 — ABBS Runtime Qualification:** qualify the integrated door on ABBS.
- **M8.3 — Second BBS Adapter / AmiExpress investigation:** study and design
  the next adapter only after ABBS experience.

Each milestone should remain independently testable and avoid prematurely
introducing background or elaborate UI infrastructure.
