/* Select an unambiguous location and make it persistent. */
OPTIONS RESULTS
OPTIONS FAILAT 21
PARSE ARG place
IF place = '' THEN DO
    SAY 'Usage: RX SetDefault.rexx "unambiguous search text"'
    EXIT 11
END
ADDRESS AMIWEATHER
RESULT = ''
'LOCATION ' || place
location_rc = RC
location_result = RESULT
SAY 'LOCATION RC=' location_rc ' RESULT=' location_result
IF location_rc ~= 0 THEN EXIT location_rc
RESULT = ''
'SETDEFAULT'
default_rc = RC
default_result = RESULT
SAY 'SETDEFAULT RC=' default_rc ' RESULT=' default_result
IF default_rc ~= 0 THEN EXIT default_rc
SAY 'Active location saved as the persistent default.'
EXIT 0
