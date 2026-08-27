# M9 manual Amiga acceptance

M9 is not fully runtime-qualified until the remaining checks below pass on the
AmigaOS 3.1 FS-UAE system. Record the tested binary hashes and observations.

## Runtime evidence already recorded

Real FS-UAE testing has confirmed:

- M9ARexx and GetWeather returned RC 0 for VERSION, STATUS, GET LOCATION,
  GET TEMPERATURE, GET CONDITION, GET HUMIDITY, and GET WIND. GetWeather
  returned live Tonstad, Agder, Norway data.
- `SetLocation.rexx "Tonstad"` returned LOCATION RC 0 and RESULT
  `Tonstad, Agder, Norway`. It changed the active location and explicitly did
  not save preferences.
- Ambiguous Reykjavik, Oslo, Manila, and New York searches returned RC 14.
  This is the required no-guessing behavior.
- Unresolved Port-Aux-Francais and Longyearbyen searches returned the
  controlled location/geocoding failure RC 15.
- The corrected scripts printed RC/RESULT, avoided the classic ARexx
  source-line abort, and returned AmiWeather's application RC to the Shell.
- `SetDefault.rexx "Tonstad"` completed LOCATION first, then SETDEFAULT with
  RC 0 and RESULT `OK`, and reported the persistent save.
- `SetDefault.rexx "Oslo"` returned LOCATION RC 14 and exited 14 without
  invoking SETDEFAULT.

The example correction is `OPTIONS FAILAT 21`: classic ARexx otherwise aborts
when a host RC reaches its default failure threshold. Each script clears RESULT,
then captures and reports RC/RESULT before deciding whether to continue.

## Reversible preference backup

Before a destructive preference test, create a host-persistent backup:

```text
MakeDir Work:M9PrefsBackup
Copy ENV:AmiWeather/preferences TO Work:M9PrefsBackup/preferences.env
Copy ENVARC:AmiWeather/preferences TO Work:M9PrefsBackup/preferences.envarc
```

A missing-source error means that copy did not exist. Restore with:

```text
MakeDir ENV:AmiWeather
MakeDir ENVARC:AmiWeather
Copy Work:M9PrefsBackup/preferences.env TO ENV:AmiWeather/preferences
Copy Work:M9PrefsBackup/preferences.envarc TO ENVARC:AmiWeather/preferences
```

Do not delete `Work:M9PrefsBackup` until qualification is complete.

## Remaining FS-UAE qualification

### A. Clean first run and reboot persistence

1. Back up preferences, then remove both live copies:

```text
Delete ENV:AmiWeather/preferences
Delete ENVARC:AmiWeather/preferences
```

2. Launch `Work:AmiWeather`. Confirm no Oslo fetch, the useful no-default
   message, and working Change location selection.
3. Quit without saving and relaunch. Confirm no default remains.
4. With the GUI running, set Tonstad persistently:

```text
RX Work:AmiWeather-ARexx/SetDefault.rexx "Tonstad"
```

5. Confirm LOCATION and SETDEFAULT both return RC 0. In GUI Preferences choose
   Fahrenheit and mph and save.
6. Confirm both copies contain Tonstad, Fahrenheit, and mph:

```text
Type ENV:AmiWeather/preferences
Type ENVARC:AmiWeather/preferences
```

7. Quit AmiWeather and perform an actual AmigaOS reboot.
8. Relaunch AmiWeather. Confirm Tonstad, Fahrenheit, and mph survived. A simple
   process restart is not sufficient for this check.

### B. Shared GUI and Door preferences / ABBS regression

9. With the persisted state from A, start ABBS 3.3 and OPEN AmiWeatherDoor.
10. Confirm the Door starts at Tonstad and displays Fahrenheit and mph.
11. Exercise H, D, 7, A, W, and R; confirm each remains usable.
12. Press Q and confirm clean return to ABBS with no Shell exposure.
13. Do not add or expect an ARexx port from AmiWeatherDoor.

### C. GUI ARexx port lifecycle

14. Start AmiWeather and prove the host is active:

```text
RX Work:AmiWeather-ARexx/M9ARexx.rexx
```

15. Confirm the commands return RC 0. Quit AmiWeather normally.
16. Run the same command and confirm AMIWEATHER is unavailable rather than
    reaching a stale port.
17. Relaunch AmiWeather, rerun the script, and confirm RC 0 responses return.

### D. Port collision

18. Keep the first AmiWeather instance running and responsive to VERSION.
19. Launch a second AmiWeather instance. This is the safest collision test with
    existing tools; do not install or write a dummy-port utility.
20. Confirm the second instance handles the occupied name without crashing and
    commands still reach the original owner.
21. Quit the second instance. Confirm the original AMIWEATHER port still works.
22. Quit the original instance and confirm the port disappears.
23. If instance identity cannot be distinguished safely, record collision
    testing as remaining rather than weakening or instrumenting production code.

### E. Optional persistence failure

24. Perform this only if ENVARC publication can be made temporarily unavailable
    without risking other files. Back up both preference copies first.
25. Attempt SAVE, confirm the GUI/ARexx reports failure, then restore access and
    confirm the previous preferences still load.
26. If no safe reversible simulation exists, record this optional test as not
    performed. Do not damage the filesystem merely to force rollback.

## Previously qualified ARexx example commands

These exact commands produced the evidence recorded above:

```text
RX Work:AmiWeather-ARexx/GetWeather.rexx
RX Work:AmiWeather-ARexx/SetLocation.rexx "Tonstad"
RX Work:AmiWeather-ARexx/SetDefault.rexx "Tonstad"
RX Work:AmiWeather-ARexx/SetLocation.rexx "Oslo"
RX Work:AmiWeather-ARexx/SetDefault.rexx "Oslo"
RX Work:AmiWeather-ARexx/SetLocation.rexx "Port-Aux-Francais"
```

Tonstad is the qualified success case. Oslo demonstrates controlled RC 14 and
SETDEFAULT gating. Port-Aux-Francais is an observed controlled RC 15 failure;
it must not be described as a successful test location.
