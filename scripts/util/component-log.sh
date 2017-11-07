#!/bin/bash
#
# Given a component and a log file, filter the log so that only the
# messages from the component are output.

if [[ $# == 0 || $# > 2 ]]; then
    echo "Error: Wrong number of arguments"
    echo "Description: Given a component and log file,"
    echo "             filter the log so that only messages"
    echo "             from the component are output."
    echo "Usage: $0 COMPONENT [LOG_FILE]"
    echo "If no log file is provided as second argument, then use stdin"
    exit 1
fi

COMPONENT="$1"
LOGFILE="$2"

readonly date_re='[[:digit:]]{4}-[[:digit:]]{2}-[[:digit:]]{2}'
readonly time_re='[[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{3}'
readonly timestamp_re="\[${date_re} ${time_re}\]"
readonly loglevel_re="\[(ERROR|WARN|INFO|DEBUG|FINE)\]"
readonly component_re="\[$COMPONENT\]"
readonly logline_re="$timestamp_re $loglevel_re .*"
readonly component_logline_re="$timestamp_re $loglevel_re $component_re .*"

keep_printing=n                 # for printing multiline messages
grep_component_lines()
{
    local line="$1"
    if [[ "$line" =~ $component_logline_re ]]; then
        keep_printing=y
    elif [[ "$line" =~ $logline_re ]]; then
        keep_printing=n
    fi
    if [[ $keep_printing == y ]]; then
        echo "$line"
    fi
}

if [[ "$LOGFILE" > 0 ]]; then
    while read; do
        grep_component_lines "$REPLY"
    done < "$LOGFILE"
else
    while read; do
        grep_component_lines "$REPLY"
    done
fi
