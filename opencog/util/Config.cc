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
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <unistd.h>

// For backward compatibility as from boost 1.46 filesystem 3 is the default
// as of boost 1.50 there is no version 2, and compiles will fail ;-(
#include <boost/version.hpp>
#if BOOST_VERSION > 104900
#define BOOST_FILESYSTEM_VERSION 3
#else
#define BOOST_FILESYSTEM_VERSION 2
#endif
#include <boost/filesystem/operations.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

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


Config* Config::createInstance()
{
    return new Config();
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
    _no_config_loaded = true;
    _had_to_search = true;
    _abs_path = "";
    _cfg_filename = "";
}

static const char* DEFAULT_CONFIG_FILENAME = "opencog.conf";
static const char* DEFAULT_CONFIG_PATHS[] =
{
    // A bunch of relative paths, typical for the current opencog setup.
    "./",
    "../",
    "../../",
    "../../../",
    "../../../../",
    "./lib/",
    "../lib/",
    "../../lib/",
    "../../../lib/",
    "../../../../lib/", // yes, really needed for some test cases!
    CONFDIR,
#ifndef WIN32
    "/etc/opencog",
    "/etc",
#endif // !WIN32
    NULL
};

const std::vector<std::string> Config::search_paths() const
{
    std::vector<std::string> paths;
    if (_had_to_search)
    {
         int i=0;
         while (DEFAULT_CONFIG_PATHS[i])
         {
             paths.push_back(DEFAULT_CONFIG_PATHS[i]);
             i++;
         }
    }
    else
    {
        paths.push_back(_abs_path);
    }
    return paths;
}

void Config::check_for_file(std::ifstream& fin,
                            const char* path_str, const char* filename)
{
    boost::filesystem::path configPath(path_str);
    configPath /= filename;

    if (not boost::filesystem::exists(configPath)) return;

    // Try to open the config file
    fin.open(configPath.string().c_str());
    if (fin and fin.good() and fin.is_open())
    {
        // XXX FIXME Allowing boost to search relative paths is
        // a security bug waiting to happen. Right now, it seems
        // like a very very unlikely thing, but it is a bug!
        if ('/' != configPath.string()[0])
        {
            char buff[PATH_MAX+1];
            char *p = getcwd(buff, PATH_MAX);
            if (p) {
                _path_where_found = buff;
                _path_where_found += '/';
            }
        }
        _path_where_found += configPath.string();
    }
}

// constructor
void Config::load(const char* filename, bool resetFirst)
{
    if (NULL == filename or 0 == filename[0])
        filename = DEFAULT_CONFIG_FILENAME;

    // Reset to default values
    if (resetFirst) reset();

    _cfg_filename = filename;

    ifstream fin;

    // If the filename specifies an absolute path, go directly there.
    if ('/' == filename[0])
    {
        _had_to_search = false;
        _abs_path = filename;
        check_for_file(fin, "", filename);
    }
    else
    {
        // Search for the filename in a bunch of typical locations.
        for (int i = 0; DEFAULT_CONFIG_PATHS[i] != NULL; ++i)
        {
            check_for_file(fin, DEFAULT_CONFIG_PATHS[i], filename);
            if (fin and fin.good() and fin.is_open())
                break;
        }
    }

    // Whoops, failed.
    if (!fin or !fin.good() or !fin.is_open())
    {
        // This will print the diagnostics to the default log file
        // location.  Note that the config file itself contains the
        // log file location that is supposed to be used, so this
        // printing is happening "too early", before the logger is
        // fully initialized. Such is life; this is still a lot easier
        // than debugging the thrown exception in a debugger.
        logger().warn("No config file found!\n");
        logger().warn("Searched for \"%s\"\n", search_file().c_str());
        for (auto& path : search_paths())
        logger().warn("Searched at %s\n", path.c_str());

        throw IOException(TRACE_INFO,
             "unable to open file \"%s\"", filename);
    }

    _no_config_loaded = false;

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
                  line_number, path_where_found().c_str());

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
    // Such is life; this is a lot easier than debugging screwed-up
    // file-path craziness in a debugger. We MUST log the path!!!
    setup_logger();

    // And then finally, at long last!!! report what happened.
    logger().info("Using config file found at: %s\n",
                  path_where_found().c_str());
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
    if (_no_config_loaded)
        logger().warn("No configuration file was loaded! Param=%s",
                      name.c_str());
    return (_table.find(name) != _table.end());
}

void Config::set(const std::string &parameter_name,
                 const std::string &parameter_value)
{
    _no_config_loaded = false;
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
    try {
        return boost::lexical_cast<int>(get(name));
    } catch (boost::bad_lexical_cast&) {
        throw InvalidParamException(TRACE_INFO,
               "[ERROR] invalid integer parameter (%s)",
               name.c_str());
    }
}

long Config::get_long(const string &name, long dfl) const
{
    if (not has(name)) return dfl;
    try {
        return boost::lexical_cast<long>(get(name));
    } catch (boost::bad_lexical_cast&) {
        throw InvalidParamException(TRACE_INFO,
               "[ERROR] invalid long integer parameter (%s)",
               name.c_str());
    }
}

double Config::get_double(const string &name, double dfl) const
{
    if (not has(name)) return dfl;
    try {
        return boost::lexical_cast<double>(get(name));
    } catch (boost::bad_lexical_cast&) {
        throw InvalidParamException(TRACE_INFO,
               "[ERROR] invalid double parameter (%s)",
               name.c_str());
    }
}

bool Config::get_bool(const string &name, bool dfl) const
{
    if (not has(name)) return dfl;
    if (boost::iequals(get(name), "true")) return true;
    else if (boost::iequals(get(name), "false")) return false;
    else throw InvalidParamException(TRACE_INFO,
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
Config& opencog::config(ConfigFactory* factoryFunction,
                        bool overwrite)
{
    static std::unique_ptr<Config> instance((*factoryFunction)());
    if (overwrite)
        instance.reset((*factoryFunction)());
    return *instance;
}
