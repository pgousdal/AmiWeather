# M9 manual Amiga acceptance

This milestone is not runtime-qualified until all items are completed on an
AmigaOS 3.1 system.

- Remove both ENV and ENVARC preferences; launch the GUI and confirm no Oslo
  fetch, useful no-default text, and working location selection.
- Select a location, use Set Current as Default, quit/relaunch, reboot, and
  confirm the same default. Save changed temperature and wind units and repeat
  the reboot/relaunch check.
- Launch AmiWeatherDoor with those preferences and confirm the same default and
  units. Remove both copies and confirm it asks for `L` rather than fetching
  Oslo. Exercise H/D/7/A/W/L/R/Q, clean ABBS return, and Shell isolation.
- From another Shell, verify `AMIWEATHER` and run VERSION, STATUS, all five GET
  queries, REFRESH, unambiguous LOCATION, SETDEFAULT, unit SET commands, and
  SAVE. Confirm GUI state changes and persistence after relaunch/reboot.
- Exercise malformed, unknown, oversized, no-state, and ambiguous LOCATION
  requests and verify their documented return codes.
- Quit normally and confirm the `AMIWEATHER` port disappears with no dangling
  messages or port.
