#!/bin/bash
#
# Given a component and a log file, filter the log so that only the
# messages from the component are output.

set -u

#############
# Constants #
#############

readonly date_re='[[:digit:]]{4}-[[:digit:]]{2}-[[:digit:]]{2}'
readonly time_re='[[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{2}:[[:digit:]]{3}'
readonly timestamp_re="\[${date_re} ${time_re}\]"
readonly loglevel_re="\[(ERROR|WARN|INFO|DEBUG|FINE)\]"
readonly logline_re="$timestamp_re $loglevel_re .*"

#############
# Functions #
#############

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

usage()
{
    echo "Description: Given a component and log file,"
    echo "             filter the log so that only messages"
    echo "             from the component are output."
    echo "Usage: $0 COMPONENT [LOG_FILE]"
    echo "If no log file is provided as second argument, then use stdin"
}

########
# Main #
########

if [[ $# == 0 || $# > 2 ]]; then
    echo "Error: Wrong number of arguments"
    usage
    exit 1
fi

if [[ $1 == "-h" || $1 == "--help" || $1 == "-?" ]]; then
    usage
    exit 0
fi

readonly COMPONENT="$1"
readonly LOGFILE="$2"
readonly component_re="\[$COMPONENT\]"
readonly component_logline_re="$timestamp_re $loglevel_re $component_re .*"

if [[ "$LOGFILE" > 0 ]]; then
    while read; do
        grep_component_lines "$REPLY"
    done < "$LOGFILE"
else
    while read; do
        grep_component_lines "$REPLY"
    done
fi
