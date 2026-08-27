/* Change the active location only; this does not save preferences. */
OPTIONS RESULTS
OPTIONS FAILAT 21
PARSE ARG place
IF place = '' THEN DO
    SAY 'Usage: RX SetLocation.rexx "unambiguous search text"'
    EXIT 11
END
ADDRESS AMIWEATHER
RESULT = ''
'LOCATION ' || place
location_rc = RC
location_result = RESULT
SAY 'LOCATION RC=' location_rc ' RESULT=' location_result
IF location_rc ~= 0 THEN EXIT location_rc
SAY 'Active location changed; preferences were not saved.'
EXIT 0
