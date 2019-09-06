INCLUDE ( CheckIncludeFile )
INCLUDE ( CheckIncludeFileCXX )
CHECK_INCLUDE_FILE_CXX ( cxxabi.h HAVE_CXXABI_H )
CHECK_INCLUDE_FILE ( execinfo.h HAVE_EXECINFO_H )

if ( HAVE_EXECINFO_H AND HAVE_CXXABI_H )
	set( GNU_BACKTRACE_FOUND TRUE )
endif ( HAVE_EXECINFO_H AND HAVE_CXXABI_H )

if ( GNU_BACKTRACE_FOUND )
	message( STATUS "Found GNU execinfo.h C++ backtrace functionality")
else ( GNU_BACKTRACE_FOUND )
	message( STATUS "GNU execinfo.h C++ backtrace functionality not found")
endif ( GNU_BACKTRACE_FOUND )
