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
- **M4 — Preferences and Saved Locations:** preferences, persistence, and
  saved-location usability.

Each milestone should remain independently testable and avoid prematurely
introducing background or elaborate UI infrastructure.
