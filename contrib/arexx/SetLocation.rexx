/* A result is accepted only when the search is unambiguous. */
OPTIONS RESULTS
PARSE ARG place
IF place = '' THEN place = 'Grimstad'
ADDRESS AMIWEATHER
'LOCATION ' || place
SAY RESULT
