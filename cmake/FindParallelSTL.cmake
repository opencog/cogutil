INCLUDE ( CheckIncludeFileCXX )
CHECK_INCLUDE_FILE_CXX ( parallel/algorithm HAVE_PARALLEL_ALGORITHM )

if ( HAVE_PARALLEL_ALGORITHM )
	set( PARALLEL_STL_FOUND TRUE )
endif ( HAVE_PARALLEL_ALGORITHM )

if ( PARALLEL_STL_FOUND )
	message( STATUS "C++ library standardizes parallelism" )
else ( PARALLEL_STL_FOUND )
	message( STATUS "Standard C++ parallelism not found" )
endif ( PARALLEL_STL_FOUND )
