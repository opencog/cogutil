# This file was borrowed from the pcsx2 - a Playstation 2 Emulator project.
# No header was provided with this file, nor was there a licence file.  
# If anyone thinks that the use of this file violates certain copyrights,
# please inform me of this matter.  - Jan Fostier, May 12 2011.

# Try to find SparseHash
# Once done, this will define
#
# SPARSEHASH_FOUND - system has SparseHash
# SPARSEHASH_INCLUDE_DIR - the SparseHash include directories

if(SPARSEHASH_INCLUDE_DIR)
	set(SPARSEHASH_FIND_QUIETLY TRUE)
endif(SPARSEHASH_INCLUDE_DIR)

find_path(SPARSEHASH_INCLUDE_DIR google/sparsehash/sparsehashtable.h)

# handle the QUIETLY and REQUIRED arguments and set SPARSEHASH_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SparseHash DEFAULT_MSG SPARSEHASH_INCLUDE_DIR)

mark_as_advanced(SPARSEHASH_INCLUDE_DIR)
