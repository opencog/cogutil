/*
 * opencog/util/Config.cc
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * All Rights Reserved
 *
 * Written by Gustavo Gama <gama@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "Config.h"
#include "Logger.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <unistd.h>

#include <opencog/util/platform.h>
#include <opencog/util/exceptions.h>

using namespace opencog;
using namespace std;

// Returns a string with leading/trailing characters of a set stripped
static char const* blank_chars = " \t\f\v\n\r";
static string strip(string const& str, char const *strip_chars = blank_chars)
{
    string::size_type const first = str.find_first_not_of(strip_chars);
    return (first == string::npos) ?
        string() :
        str.substr(first, str.find_last_not_of(strip_chars) - first + 1);
}

Config::~Config()
{
}

Config::Config()
{
    reset();
}

void Config::reset()
{
    _table.clear();
}

static const char* DEFAULT_CONFIG_FILENAME = "opencog.conf";

void Config::load(const char* filename)
{
    if (NULL == filename or 0 == filename[0])
        filename = DEFAULT_CONFIG_FILENAME;

    std::filesystem::path configPath(filename);
    if (not std::filesystem::exists(configPath))
    {
        logger().warn("Can't find config file \"%s\"\n", filename);
        throw IOException(TRACE_INFO,
             "Can't find config file \"%s\"", filename);
    }

    // Try to open it
    ifstream fin;
    fin.open(filename);
    if (!fin or !fin.good() or !fin.is_open())
    {
        logger().warn("Cannot open config file \"%s\"\n", filename);
        throw IOException(TRACE_INFO,
             "Unable to open config file \"%s\"", filename);
    }

    string line;
    string name;
    string value;
    unsigned int line_number = 0;
    bool have_name = false;
    bool have_value = false;

    // Read and parse the config file.
    while (++line_number, fin.good() and getline(fin, line))
    {
        string::size_type idx;

        // Find comment and discard the rest of the line.
        if ((idx = line.find('#')) != string::npos) {
            line.replace(idx, line.size() - idx, "");
        }

        // Search for the '=' character.
        if ((idx = line.find('=')) != string::npos)
        {
            // Select name and value
            name  = line.substr(0, idx);
            value = line.substr(idx + 1);

            // Strip out whitespace, etc.
            name  = strip(name);
            value = strip(value);
            value = strip(value, "\"");

            have_name = true;

            // Look for trailing comma.
            if (',' != value[value.size()-1])
                have_value = true;
        }

        // Append more to value
        else if ((string::npos != line.find_first_not_of(blank_chars)) &&
            have_name &&
            !have_value)
        {
            value += strip(line);

            // Look for trailing comma.
            if (',' != value[value.size()-1])
                have_value = true;
        }

        else if (line.find_first_not_of(blank_chars) != string::npos)
        {
            // This might print the diagnostics to the default log file
            // location.  Note that the config file itself contains the
            // log file location that is supposed to be used, so this
            // printing is happening "too early", before the logger is
            // fully initialized. Such is life; this is still a lot easier
            // than debugging the thrown exception in a debugger.
            setup_logger();
            logger().warn("Invalid config file entry at line %d in %s\n",
                  line_number, filename);

            throw InvalidParamException(TRACE_INFO,
                  "[ERROR] invalid configuration entry (line %d)",
                  line_number);
        }

        if (have_name && have_value)
        {
            // Finally, store the entries.
            _table[name] = value;
            have_name = false;
            have_value = false;
            value = "";
        }
    }
    fin.close();

    // Finish configuring the logger... The config file itself
    // contains the location of the log file. This is working around
    // a chicken-and-egg problem with reporting config file issues.
    setup_logger();
}

void Config::setup_logger()
{
    if (has("LOG_FILE"))
        logger().set_filename(get("LOG_FILE"));
    if (has("LOG_LEVEL"))
        logger().set_level(get("LOG_LEVEL"));
    if (has("BACK_TRACE_LOG_LEVEL"))
        logger().set_backtrace_level(get("BACK_TRACE_LOG_LEVEL"));
    if (has("LOG_TO_STDOUT"))
        logger().set_print_to_stdout_flag(get_bool("LOG_TO_STDOUT"));
    if (has("LOG_TIMESTAMP"))
        logger().set_timestamp_flag(get_bool("LOG_TIMESTAMP"));
}

const bool Config::has(const string &name) const
{
    return (_table.find(name) != _table.end());
}

void Config::set(const std::string &parameter_name,
                 const std::string &parameter_value)
{
    _table[parameter_name] = parameter_value;
}

const string& Config::get(const string& name, const string& dfl) const
{
    if (not has(name)) return dfl;
    return _table.find(name)->second;
}

const string& Config::operator[](const string &name) const
{
    if (not has(name))
       throw InvalidParamException(TRACE_INFO,
                                   "[ERROR] parameter not found (%s)",
                                   name.c_str());
    return _table.find(name)->second;
}

int Config::get_int(const string &name, int dfl) const
{
    if (not has(name)) return dfl;
    return std::stoi(get(name));
}

long Config::get_long(const string &name, long dfl) const
{
    if (not has(name)) return dfl;
    return std::stol(get(name));
}

double Config::get_double(const string &name, double dfl) const
{
    if (not has(name)) return dfl;
    return std::stod(get(name));
}

bool Config::get_bool(const string &name, bool dfl) const
{
    if (not has(name)) return dfl;
    if (0 == get(name).compare("true")) return true;
    if (0 == get(name).compare("false")) return false;
    throw InvalidParamException(TRACE_INFO,
                "[ERROR] invalid bool parameter (%s: %s)",
                name.c_str(), get(name).c_str());
}

std::string Config::to_string() const
{
    std::ostringstream oss;
    oss << "{\"";
    for (auto it = _table.begin(); it != _table.end(); ++it) {
        if (it != _table.begin()) oss << "\", \"";
        oss << it->first << "\" => \"" << it->second;
    }
    oss << "\"}";
    return oss.str();
}

// create and return the single instance
Config& opencog::config()
{
    static std::unique_ptr<Config> instance(new Config());
    return *instance;
}
