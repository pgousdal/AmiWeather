/* Select an unambiguous location and make it persistent. */
OPTIONS RESULTS
PARSE ARG place
IF place = '' THEN place = 'Grimstad'
ADDRESS AMIWEATHER
'LOCATION ' || place
SAY RESULT
'SETDEFAULT'
SAY RESULT
