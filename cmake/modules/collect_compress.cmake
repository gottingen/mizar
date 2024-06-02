#
# Copyright (C) 2024 EA group inc.
# Author: Jeff.li lijippy@163.com
# All rights reserved.
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

set(EA_INF /opt/EA/inf)

include(GNUInstallDirs)
set(EA_INCLUDE_DIR ${EA_INF}/${CMAKE_INSTALL_INCLUDEDIR})
set(EA_LIB_DIR ${EA_INF}/${CMAKE_INSTALL_LIBDIR})

set(MIZAR_COMPRESS_LIBS)
####################################################################################################
# zlib
####################################################################################################
find_path(ZLIB_INCLUDE_DIR zlib.h HINTS ${EA_INCLUDE_DIR})
find_library(ZLIB_LIBRARY NAMES z HINTS ${EA_LIB_DIR})

if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
    set(ZLIB_FOUND TRUE)
    message(STATUS "Found ZLIB: ${ZLIB_LIBRARY}")
    list(APPEND MIZAR_COMPRESS_LIBS ${ZLIB_LIBRARY})
else()
    set(ZLIB_FOUND FALSE)
    message(FATAL_ERROR "ZLIB not found")
endif()
list(APPEND MIZAR_COMPRESS_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
list(APPEND MIZAR_COMPRESS_LIBS ${ZLIB_LIBRARY})

####################################################################################################
# zstd
####################################################################################################
find_path(ZSTD_INCLUDE_DIR zstd.h HINTS ${EA_INCLUDE_DIR})
find_library(ZSTD_LIBRARY NAMES zstd HINTS ${EA_LIB_DIR})

if(ZSTD_INCLUDE_DIR AND ZSTD_LIBRARY)
    set(ZSTD_FOUND TRUE)
    message(STATUS "Found ZSTD: ${ZSTD_LIBRARY}")
    list(APPEND MIZAR_COMPRESS_LIBS ${ZSTD_LIBRARY})
else()
    set(ZSTD_FOUND FALSE)
    message(FATAL_ERROR "ZSTD not found")
endif()

list(APPEND MIZAR_COMPRESS_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
list(APPEND MIZAR_COMPRESS_LIBS ${ZSTD_LIBRARY})

####################################################################################################
# lz4
####################################################################################################
find_path(LZ4_INCLUDE_DIR lz4.h HINTS ${EA_INCLUDE_DIR})
find_library(LZ4_LIBRARY NAMES lz4 HINTS ${EA_LIB_DIR})

if(LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
    set(LZ4_FOUND TRUE)
    message(STATUS "Found LZ4: ${LZ4_LIBRARY}")
    list(APPEND MIZAR_COMPRESS_LIBS ${LZ4_LIBRARY})
else()
    set(LZ4_FOUND FALSE)
    message(FATAL_ERROR "LZ4 not found")
endif()

list(APPEND MIZAR_COMPRESS_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})
list(APPEND MIZAR_COMPRESS_LIBS ${LZ4_LIBRARY})

####################################################################################################
