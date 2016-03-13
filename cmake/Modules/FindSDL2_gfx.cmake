# Locate SDL2_gfx library
#
# This module defines:
#
# ::
#
#   SDL2_GFX_LIBRARIES, the name of the library to link against
#   SDL2_GFX_INCLUDE_DIRS, where to find the headers
#   SDL2_GFX_FOUND, if false, do not try to link against
#   SDL2_GFX_VERSION_STRING - human-readable string containing the version of SDL2_gfx
#
#
#
# $SDL2DIR is an environment variable that would correspond to the
# ./configure --prefix=$SDL2DIR used in building SDL2.
#
# Created by Eric Wing.  This was influenced by the FindSDL.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(SDL2_GFX_INCLUDE_DIR SDL2_gfxPrimitives.h
  HINTS
    ENV SDL2GFXDIR
    ENV SDL2DIR
  PATH_SUFFIXES SDL2
                # path suffixes to search inside ENV{SDL2DIR}
                include/SDL2 include
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

find_library(SDL2_GFX_LIBRARY
  NAMES SDL2_gfx
  HINTS
    ENV SDL2GFXDIR
    ENV SDL2DIR
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

if(SDL2_GFX_INCLUDE_DIR AND EXISTS "${SDL2_GFX_INCLUDE_DIR}/SDL2_gfxPrimitives.h")
  file(STRINGS "${SDL2_GFX_INCLUDE_DIR}/SDL2_gfxPrimitives.h" SDL2_GFX_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL2_GFXPRIMITIVES_MAJOR[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_GFX_INCLUDE_DIR}/SDL2_gfxPrimitives.h" SDL2_GFX_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL2_GFXPRIMITIVES_MINOR[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_GFX_INCLUDE_DIR}/SDL2_gfxPrimitives.h" SDL2_GFX_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL2_GFXPRIMITIVES_MICRO[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL2_GFXPRIMITIVES_MAJOR[ \t]+([0-9]+)$" "\\1" SDL2_GFX_VERSION_MAJOR "${SDL2_GFX_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL2_GFXPRIMITIVES_MINOR[ \t]+([0-9]+)$" "\\1" SDL2_GFX_VERSION_MINOR "${SDL2_GFX_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL2_GFXPRIMITIVES_MICRO[ \t]+([0-9]+)$" "\\1" SDL2_GFX_VERSION_PATCH "${SDL2_GFX_VERSION_PATCH_LINE}")
  set(SDL2_GFX_VERSION_STRING ${SDL2_GFX_VERSION_MAJOR}.${SDL2_GFX_VERSION_MINOR}.${SDL2_GFX_VERSION_PATCH})
  unset(SDL2_GFX_VERSION_MAJOR_LINE)
  unset(SDL2_GFX_VERSION_MINOR_LINE)
  unset(SDL2_GFX_VERSION_PATCH_LINE)
  unset(SDL2_GFX_VERSION_MAJOR)
  unset(SDL2_GFX_VERSION_MINOR)
  unset(SDL2_GFX_VERSION_PATCH)
endif()

set(SDL2_GFX_LIBRARIES ${SDL2_GFX_LIBRARY})
set(SDL2_GFX_INCLUDE_DIRS ${SDL2_GFX_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_gfx
                                  REQUIRED_VARS SDL2_GFX_LIBRARIES SDL2_GFX_INCLUDE_DIRS
                                  VERSION_VAR SDL2_GFX_VERSION_STRING)

mark_as_advanced(SDL2_GFX_LIBRARY SDL2_GFX_INCLUDE_DIR)
