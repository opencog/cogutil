#!/usr/bin/env python

# Sort a log according to some order (chronological or thread
# cohesiveness for now).

import re
import sys
import datetime
import argparse
import functools

def datetime_from_str(time_str):
    """Return <datetime.datetime() instance> for the given
    datetime string given in OpenCog's date time log format.

    >>> datetime_from_str("2009-12-25 13:05:14:453")
    datetime.datetime(2009, 12, 25, 13, 5, 14, 453000)
    """
    fmt = "%Y-%m-%d %H:%M:%S:%f"
    return datetime.datetime.strptime(time_str, fmt)

def compare(x, y):
    if x < y:
        return -1
    elif x > y:
        return 1
    else:
        return 0

def chrono_compare(x, y):
    """Compare keys x and y so that

    x = (xtln, xdt, xln)
    y = (ytln, ydt, yln)

    ignoring thread line number, that is compare

    (xdt, xln) and (ydt, xln)
    """
    xtln, xdt, xln = x
    ytln, ydt, yln = y

    return compare((xdt, xln), (ydt, yln))

def thread_compare(x, y):
    """Compare keys x and y so that

    x = (xtln, xdt, xln)
    y = (ytln, ydt, yln)

    1. If xtln or ytln is None, or xtln == ytln then ignore the first
    two fields, that is compare with line number.

    2. If xtln and ytln are not None, and xtln != ytln then compare
    with thread line number and line number.

    """
    xtln, xdt, xln = x
    ytln, ydt, yln = y

    if xtln and ytln and xtln != ytln:
        return compare((xtln, xln), (ytln, yln))
    else:
        return compare(xln, yln)

def ln_compare(x, y):
    """Compare keys x and y

    x = (xtln, xdt, xln)
    y = (ytln, ydt, yln)

    by line number.
    """
    xtln, xdt, xln = x
    ytln, ydt, yln = y

    return compare(xln, yln)

def thread_chrono_compare(x, y):
    """Compare keys x and y so that

    x = (xtln, xdt, xln)
    y = (ytln, ydt, yln)

    1. If xtln or ytln is None, or xtln == ytln then ignore the first
    field, that is compare with the remaining fields datetime and line
    number.

    2. If xtln and ytln are not None, and xtln != ytln then compare
    the whole triplet with the standard order.

    """
    xtln, xdt, xln = x
    ytln, ydt, yln = y

    if xtln and ytln and xtln != ytln:
        return compare(x, y)
    else:
        return chrono_compare(x, y)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Sort the given log according to a given order, such as chronological, thread cohesive, etc.')
    parser.add_argument('logfile', help='Log file to sort')
    parser.add_argument('-c', '--chrono', action='store_true', default=True,
                        help='Sort chronologically. Indeed, such order can be broken if the logger is asynchronous.')
    parser.add_argument('-t', '--thread', action='store_true', default=False,
                        help='Sort such that messages from the same thread are clumped together.')
    parser.add_argument('-o', '--output',
                        help='Output file. If unused stdout is used instead')
    args = parser.parse_args()

    timestamp_re = r'\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}:\d{3})\]'
    timestamp_prog = re.compile(timestamp_re)
    thread_re = r'.*\[(thread-\d+)\]'
    thread_prog = re.compile(thread_re)

    # Output file
    of = open(args.output, "w") if args.output else sys.stdout
    
    # 1. Put the text in a dict (tln, dt, ln) -> message
    #
    #    where
    #
    #    - tln is the first line number of its corresponding thread
    #    - dt is the corresponding datetime
    #    - ln is the line number of the first line of the message
    # 
    # 2. Map thread to its first line number
    key2txt = {}
    tln, dt, ln = None, None, None
    line_num = 0
    t2ln = {}
    for l in open(args.logfile):
        # Parse timestamp, thread and fill the key2txt entry
        timestamp_m = timestamp_prog.match(l)
        thread_m = thread_prog.match(l)
        if timestamp_m:
            # Determine message line number
            ln = line_num
            # Determine thread line number, if any
            if thread_m:
                thread = thread_m.group(1)
                if thread not in t2ln:
                    t2ln[thread] = ln
                tln = t2ln[thread]    
            else:
                tln = None
            # Determine datetime
            dt = datetime_from_str(timestamp_m.group(1))
            key2txt[(tln, dt, ln)] = l
        elif dt:
            key2txt[(tln, dt, ln)] += l
        else:
            # Corner case, it seems the log's first line can be without
            # timestamp
            of.write(l)
        line_num += 1

    # Determine the right compare function
    if args.chrono and args.thread:
        cmp = thread_chrono_compare
    elif args.chrono:
        cmp = chrono_compare
    elif args.thread:
        cmp = thread_compare
    else:
        cmp = ln_compare

    # Write sorted log to output
    for tln, dt, ln in sorted(key2txt.keys(), key=functools.cmp_to_key(cmp)):
        of.write(key2txt[(tln, dt, ln)])
