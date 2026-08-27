#!/bin/sh
set -eu

for script in contrib/arexx/GetWeather.rexx \
              contrib/arexx/SetLocation.rexx \
              contrib/arexx/SetDefault.rexx \
              tests/manual/M9ARexx.rexx; do
    grep -q '^OPTIONS RESULTS$' "$script"
    grep -q '^OPTIONS FAILAT 21$' "$script"
done

! grep -q "THEN place =" contrib/arexx/SetLocation.rexx
! grep -q "THEN place =" contrib/arexx/SetDefault.rexx
! grep -q "'SETDEFAULT'" contrib/arexx/SetLocation.rexx
grep -q "IF location_rc ~= 0 THEN EXIT location_rc" \
    contrib/arexx/SetLocation.rexx
grep -q "IF location_rc ~= 0 THEN EXIT location_rc" \
    contrib/arexx/SetDefault.rexx
grep -q "IF default_rc ~= 0 THEN EXIT default_rc" \
    contrib/arexx/SetDefault.rexx

location_line=$(grep -n "^'LOCATION ' || place$" \
    contrib/arexx/SetDefault.rexx | cut -d: -f1)
gate_line=$(grep -n '^IF location_rc ~= 0 THEN EXIT location_rc$' \
    contrib/arexx/SetDefault.rexx | cut -d: -f1)
default_line=$(grep -n "^'SETDEFAULT'$" \
    contrib/arexx/SetDefault.rexx | cut -d: -f1)
test "$location_line" -lt "$gate_line"
test "$gate_line" -lt "$default_line"

echo "AmiWeather ARexx example checks passed"
