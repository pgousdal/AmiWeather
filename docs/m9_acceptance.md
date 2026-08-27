# M9 manual Amiga acceptance

This milestone is not runtime-qualified until every applicable check below is
completed on the AmigaOS 3.1 FS-UAE system. Record observed RC/RESULT values and
the hashes of the tested binaries.

## Recorded FS-UAE ARexx example retest

The corrected classic ARexx examples passed their real FS-UAE retest. This
qualifies the example-script error handling, not the remaining M9 phases.

- M9ARexx and GetWeather completed with RC 0 for VERSION, STATUS, location,
  temperature, condition, humidity, and wind queries. GetWeather returned live
  Tonstad, Agder, Norway data.
- `SetLocation.rexx "Tonstad"` returned LOCATION RC 0 and RESULT
  `Tonstad, Agder, Norway`, changed only the active location, and reported that
  preferences were not saved.
- Ambiguous Oslo, Reykjavik, and New York searches returned RC 14. Unresolved
  Port-Aux-Francais and Longyearbyen searches returned RC 15. The scripts
  printed the captured RC/RESULT, avoided the classic ARexx source-line abort,
  and returned AmiWeather's RC to the Shell.
- `SetDefault.rexx "Tonstad"` completed LOCATION first, then SETDEFAULT with
  RC 0 and RESULT `OK`, and reported that Tonstad was saved persistently.
- `SetDefault.rexx "Oslo"` returned LOCATION RC 14 and exited 14 without
  invoking SETDEFAULT. This confirms the failure gate and preserves the
  production ambiguity rule.

The correction is `OPTIONS FAILAT 21`: classic ARexx otherwise aborts a script
when the host command RC reaches its default failure threshold. Each script
also clears RESULT before dispatch, then captures and prints RC/RESULT before
making an explicit success/failure decision.

## Preparation and reversible preference backup

The prepared host files map to `Work:AmiWeather`,
`ABBS:Doors/AmiWeather/AmiWeatherDoor`, and `Work:AmiWeather-ARexx/`.

Before removing preferences, create `Work:M9PrefsBackup` and copy each existing
file separately if it exists:

```text
MakeDir Work:M9PrefsBackup
Copy ENV:AmiWeather/preferences TO Work:M9PrefsBackup/preferences.env
Copy ENVARC:AmiWeather/preferences TO Work:M9PrefsBackup/preferences.envarc
```

A missing-source error is harmless and means that copy did not exist. Do not
delete the backup until qualification is complete. Remove the live test copies:

```text
Delete ENV:AmiWeather/preferences
Delete ENVARC:AmiWeather/preferences
```

## Phase A: clean first run

1. Launch `Work:AmiWeather`.
2. Confirm Oslo is not fetched and the window says `No default location configured.`
3. Confirm Change opens the normal Open-Meteo location search and select a location.
4. Quit without using Set Current as Default or Save.
5. Relaunch and confirm there is still no default and no Oslo fetch.

## Phase B: persistence

6. Select a location and choose Location / Set Current as Default.
7. Use `Type ENV:AmiWeather/preferences` and
   `Type ENVARC:AmiWeather/preferences`; confirm both exist and describe the
   selected location/default.
8. Quit and relaunch; confirm that location loads automatically.
9. Change temperature and wind units in Preferences and save them.
10. Quit and relaunch; confirm both units persist.
11. Reboot the emulated Amiga. This step is mandatory.
12. Relaunch and confirm the same default and both units survived the reboot.

## Phase C: shared Door state and ABBS regression

13. Start ABBS 3.3 and OPEN the AmiWeather door.
14. Confirm its initial location and units match the GUI preferences.
15. Exercise H, D, 7, A, W, L, and R, then Q; confirm clean ABBS return.
16. Back up the now-qualified preference files again, remove both live copies,
    and OPEN the Door.
17. Confirm no Oslo fetch, the no-default/Press L guidance, and usable L search.
18. Q cleanly, then restore both preference files with:

```text
MakeDir ENV:AmiWeather
MakeDir ENVARC:AmiWeather
Copy Work:M9PrefsBackup/preferences.env TO ENV:AmiWeather/preferences
Copy Work:M9PrefsBackup/preferences.envarc TO ENVARC:AmiWeather/preferences
```

## Phase D: ARexx commands

19. Start the GUI and run `RX Work:AmiWeather-ARexx/M9ARexx.rexx`. Its seven
    commands must all show RC 0 and sensible RESULT values; this also proves the
    public `AMIWEATHER` host is reachable.
20. Retest the corrected successful examples exactly as follows. Tonstad is the
    location that passed this qualification run.

```text
RX Work:AmiWeather-ARexx/GetWeather.rexx
RX Work:AmiWeather-ARexx/SetLocation.rexx "Tonstad"
RX Work:AmiWeather-ARexx/GetWeather.rexx
RX Work:AmiWeather-ARexx/SetDefault.rexx "Tonstad"
```

    SetLocation must print `LOCATION RC= 0`, change the active report, and not
    save preferences. After its GetWeather check, quit and relaunch the GUI;
    confirm the earlier persistent default returns. Then run SetDefault with
    the GUI open. It must print successful LOCATION and SETDEFAULT lines before
    reporting persistence. Quit and relaunch once more and confirm
    Tonstad is now the persistent default.

    Then exercise the legitimate ambiguity path:

```text
RX Work:AmiWeather-ARexx/SetLocation.rexx "Oslo"
RX Work:AmiWeather-ARexx/SetDefault.rexx "Oslo"
```

    Each must print `LOCATION RC= 14`, print the captured RESULT, and exit 14
    without an uncontrolled ARexx source-line abort. SetDefault must not run or
    print SETDEFAULT after that failure. Confirm the active location/default
    remains unchanged. The observed controlled RC 15 path can be repeated with:

```text
RX Work:AmiWeather-ARexx/SetLocation.rexx "Port-Aux-Francais"
```

    It must report and return RC 15 without a source-line abort. It is not a
    qualified successful location query.
21. Individually exercise REFRESH, an unambiguous LOCATION, SET TEMPUNIT F,
    SET WINDUNIT MPH, SAVE, and SETDEFAULT. Confirm RC 0, GUI state changes,
    and persisted full preferences.
22. Use a deliberately broad ambiguous query. Confirm RC 14 and that the prior
    active location/report remain unchanged.
23. Send `BOGUS` (RC 10), `SET TEMPUNIT X` (RC 11), and a 256-byte command if
    practical (RC 11). Confirm no crash. With no report/location, confirm the
    applicable GET and REFRESH commands return RC 12 and 13.
24. If a location search/network request fails, confirm RC 15. If saving is
    made to fail safely, confirm RC 16. An unavailable callback/state must use
    RC 17. Do not damage the filesystem merely to force these paths.

## Phase E: port lifecycle

25. Quit normally and confirm an ARexx command reports that host AMIWEATHER is
    unavailable.
26. Relaunch, rerun VERSION, and confirm RC 0 and the expected RESULT.
27. Launch a second AmiWeather instance. Confirm it handles the name collision
    without crashing and that commands still reach the original owner.
28. Quit the second instance and confirm the original port still works. Quit
    the original and confirm the port disappears.
29. Attempt a pending-message shutdown test only if a safe repeatable ARexx
    sender is available; otherwise record it as not performed.

## Optional safe failure injection

Only if the test filesystem can be made temporarily unwritable without risking
other data, block ENVARC publication, invoke SAVE, verify RC 16/UI failure, and
confirm the previous preferences still load. Restore access immediately. If no
safe simulation is available, record this check as not performed; do not use a
destructive rollback experiment.
