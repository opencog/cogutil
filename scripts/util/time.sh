#!/bin/bash

# Take a log file and return the difference in hour:min:sec between
# the timestamps of the first and last logs. This is convenient to
# estimate the runtime of some process. Will not work if the time if
# greater than 24h!

if [ $# != 1 ]; then
    echo "Wrong number of arguments"
    echo "Usage: $0 LOGFILE"
    exit 1
fi

LOGFILE="$1"

bash_timestamps_re='\[([[:digit:]]{4}-[[:digit:]]{2}-[[:digit:]]{2} [[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}):[[:digit:]]{3}'
grep_timestamps_re='\[[[:digit:]]\{4\}-[[:digit:]]\{2\}-[[:digit:]]\{2\} [[:digit:]]\{2\}:[[:digit:]]\{2\}:[[:digit:]]\{2\}:[[:digit:]]\{3\}'

FIRST_DATE=""
LAST_DATE=""
while read -r line; do
    if [[ $line =~ $bash_timestamps_re ]]; then
        if [[ -z "$FIRST_DATE" ]]; then
            FIRST_DATE=${BASH_REMATCH[1]}
        fi
        LAST_DATE=${BASH_REMATCH[1]}
    fi
done < <(grep "$grep_timestamps_re" "$LOGFILE")

FIRST_SEC="$(date -d "$FIRST_DATE" +%s)"
LAST_SEC="$(date -d "$LAST_DATE" +%s)"
DIFF_SEC="$((LAST_SEC - FIRST_SEC))"

date -u -d @${DIFF_SEC} +"%T"
