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
readonly anylevel_re="\[(ERROR|WARN|INFO|DEBUG|FINE)\]"

#############
# Functions #
#############

keep_printing=n                 # for printing multiline messages
grep_component_lines()
{
    local line="$1"
    if [[ "$line" =~ $inline_re ]]; then
        keep_printing=y
    elif [[ "$line" =~ $outline_re ]]; then
        keep_printing=n
    fi
    if [[ $keep_printing == y ]]; then
        echo "$line"
    fi
}

usage()
{
    echo "Description: Filter log file."
    echo "Usage: $0 [-h|--help|-?] [-l LEVEL] [-c COMPONENT] [LOG_FILE]"
    echo " -l Filter by log level. Only messages of level LEVEL are kept."
    echo " -c Filter by component name. Only messages from COMPONENT are kept."
    echo " -h|--help|-? Show this message."
    echo "If both -l and -c are used only messages of that level and component are kept."
    echo "If LOG_FILE is not provided then stdin is used."
}

########
# Main #
########

# Parse command arguments

if [[ $# == 0 || $# > 5 ]]; then
    echo "Error: Wrong number of arguments"
    usage
    exit 1
fi

if [[ $1 == "-h" || $1 == "--help" || $1 == "-?" ]]; then
    usage
    exit 0
fi

LEVEL=""
COMPONENT=""
UNKNOWN_FLAGS=""
while getopts "l:c:" flag ; do
    case $flag in
        l) LEVEL="${OPTARG^^}" ;;
        c) COMPONENT="$OPTARG" ;;
        *) UNKNOWN_FLAGS=true ;;
    esac
done

shift $((OPTIND-1))

set +u
LOGFILE="$1"
set -u

# Filter

if [[ -n "$LEVEL" ]]; then
    readonly level_re="\[$LEVEL\]"
else
    readonly level_re="$anylevel_re"
fi
readonly outline_re="$timestamp_re $anylevel_re .*"

if [[ -n "$COMPONENT" ]]; then
    readonly component_re="\[$COMPONENT\]"
    readonly inline_re="$timestamp_re $level_re $component_re .*"
else
    readonly inline_re="$timestamp_re $level_re .*"
fi

if [[ "$LOGFILE" > 0 ]]; then
    while read; do
        grep_component_lines "$REPLY"
    done < "$LOGFILE"
else
    while read; do
        grep_component_lines "$REPLY"
    done
fi
