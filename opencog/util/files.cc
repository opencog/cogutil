/*
 * opencog/util/files.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
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

#include <fstream>
#include <iostream>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>
#include <stdlib.h>

#include "files.h"
#include "platform.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#ifdef WIN32_NOT_UNIX
#include <direct.h>
#define  mkdir _mkdir
#endif

#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif


#define USER_FLAG "$USER"

/**
 * The file paths here have to be able to find both shared libraries
 * (dynamicaly loadable libraries) as well as scheme modules and files.
 *
 * The basic search algo is that anything in the user's directory is
 * explored first, before anything in the system directories.
 *
 * The user can set the environmental OPENCOG_MODULE_PATHS to add more
 * module paths after compile time.
 */
static const std::vector<std::string> paths =
{
#ifndef WIN32
    // XXX FIXME Searching the current path is a security breach just
    // waiting to happen, but the current OpenCog cogserver and Config
    // and unit-test-case design more or less demands this. The unit
    // tests and/or the config infrastructure would need to be overhauled.
    //
    "./",
    "../",
    "../../",
    "../../../",
    "../../../../",   // some unit tests need this
#endif // !WIN32
    CMAKE_INSTALL_PREFIX "/lib",
    CMAKE_INSTALL_PREFIX "/share",
    DATADIR,         // this too is an install dir
#ifndef WIN32
    "/usr/local/lib64/",  // lib64 is used on CentOS systems.
    "/usr/local/lib/",    // search local first, then system.
    "/usr/local/share/",  // search local first, then system.
    "/usr/lib64/",
    "/usr/lib/",
    "/usr/share/",
    "/opt/",
    "/",
#endif // !WIN32
};
const std::vector<std::string> opencog::DEFAULT_MODULE_PATHS = paths;

std::vector<std::string> opencog::get_module_paths()
{
	std::vector<std::string> results = opencog::DEFAULT_MODULE_PATHS;
	if (const char* env_p = std::getenv("OPENCOG_MODULE_PATHS"))
	{
		std::vector<std::string> split_paths;
		std::string paths(env_p);
		boost::split(split_paths, paths, boost::is_any_of(":"));
		results.insert(results.end(), split_paths.begin(), split_paths.end());
	}
	return results;
}

bool opencog::file_exists(const char* filename)
{
    std::fstream dumpFile(filename, std::ios::in);
    dumpFile.close();

    if (dumpFile.fail()) {
        dumpFile.clear(std::ios_base::failbit);
        return false;
    }
    return true;
}

bool opencog::exists(const char *fname)
{
    FILE* f = fopen(fname, "rb");
    if (!f)
        return false;
    fclose(f);
    return true;
}

void opencog::expand_path(std::string& path)
{

    size_t user_index = path.find(USER_FLAG, 0);
    if (user_index != std::string::npos) {
        const char* username = getUserName();
        path.replace(user_index, strlen(USER_FLAG), username);
    }

    return;
}

bool opencog::create_directory(const char* directory)
{

#ifdef WIN32_NOT_CYGWIN
    if (mkdir(directory) == 0 || errno == EEXIST) {
#else
    if (mkdir(directory, S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
#endif
        return true;
    }
    return false;
}

bool opencog::append_file_content(const char* filename, std::string &s)
{
    std::ifstream in(filename);
    if (!in.is_open())
        return false;

    char c;
    std::string str;
    while (in.get(c))
        str += c;

    if (!in.eof())
        return false;

    s = str;
    return true;
}

bool opencog::load_text_file(const std::string &fname, std::string& dest)
{
    FILE *f = fopen(fname.c_str(), "rt");
    if (f == NULL) {
        puts("File not found.");
        return false;
    }
    fseek(f, 0L, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    char *buf = new char[fsize+2];
    long bptr = 0;

    while (!feof(f))
        buf[bptr++] = getc(f);
// fread(buf, 8000, 1+(fsize/8000), f);
    buf[bptr] = '\0';

    fclose(f);

    dest = buf;

    delete[] buf;

    return true;
}

std::string get_exe_name()
{
    static char buf[PATH_MAX];
    int rslt = readlink("/proc/self/exe", buf, PATH_MAX);

    if ( rslt < 0 || rslt >= PATH_MAX ) {
        return NULL;
    }

    buf[rslt] = '\0';
        return std::string(buf);
}

std::string get_exe_dir()
{
    std::string exeName = get_exe_name();
    return exeName.substr(0, exeName.rfind("/")+1);
}

