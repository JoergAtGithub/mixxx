#.rst:
# FindFFmpeg
# ----------
#
# Try to find the required FFmpeg components (default: AVCODEC, AVFORMAT, AVUTIL)
#
# Next variables can be used to hint FFmpeg libs search:
#
# ::
#
#   PC_FFmpeg_LIBRARY_DIRS
#   PC_FFmpeg_INCLUDE_DIRS
#
# Once done this will define
#
# ::
#
#   FFmpeg_FOUND         - System has all required components.
#   FFmpeg_INCLUDE_DIRS  - Include directories for all required components.
#   FFmpeg_LIBRARIES     - Libraries to link for all required components.
#   FFmpeg_DEFINITIONS   - Compiler switches required for using FFmpeg.
#
# For each of the components it will additionally set.
#
# ::
#
#   FFmpeg_<COMPONENT>_FOUND        - System has <COMPONENT>
#   FFmpeg_<COMPONENT>_INCLUDE_DIRS - Include directories for <COMPONENT>
#   FFmpeg_<COMPONENT>_LIBRARIES    - Libraries to link for <COMPONENT>
#   FFmpeg_<COMPONENT>_DEFINITIONS  - Compiler switches for <COMPONENT>
#   FFmpeg_<COMPONENT>_VERSION      - Version of <COMPONENT>
#
# The following imported targets are created:
#
# ::
#
#   FFmpeg::FFmpeg     - interface target aggregating all found components
#   FFmpeg::avcodec    - libavcodec
#   FFmpeg::avformat   - libavformat
#   FFmpeg::avdevice   - libavdevice
#   FFmpeg::avutil     - libavutil
#   FFmpeg::avfilter   - libavfilter
#   FFmpeg::swscale    - libswscale
#   FFmpeg::swresample - libswresample
#
# Component names are Qt-compliant uppercase (AVCODEC, AVFORMAT, …) as
# required by Qt6's internal find_dependency() calls in
# Qt6FFmpegMediaPluginImplPrivateDependencies.cmake:
#
#   find_dependency(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL SWRESAMPLE SWSCALE)
#
# Imported target names are lowercase (FFmpeg::avcodec, …) as listed in
# Qt6FFmpegMediaPluginImplPrivateDependencies.cmake:
#
#   provided_targets "FFmpeg::avcodec;FFmpeg::avformat;FFmpeg::avutil;
#                     FFmpeg::swresample;FFmpeg::swscale"
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
# Copyright (c) 2017, Alexander Drozdov, <adrozdoff@gmail.com>
# Copyright (c) 2019, Jan Holthuis, <holthuis.jan@googlemail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

# Default components when none are requested
if(NOT FFmpeg_FIND_COMPONENTS)
  set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVUTIL)
endif()

# Maps uppercase component name → lowercase library stem (used for both the
# imported target suffix and the pkg-config / find_library name)
set(_FFmpeg_AVCODEC_lower avcodec)
set(_FFmpeg_AVFORMAT_lower avformat)
set(_FFmpeg_AVDEVICE_lower avdevice)
set(_FFmpeg_AVUTIL_lower avutil)
set(_FFmpeg_AVFILTER_lower avfilter)
set(_FFmpeg_SWSCALE_lower swscale)
set(_FFmpeg_SWRESAMPLE_lower swresample)

# Maps uppercase component name → pkg-config module name
set(_FFmpeg_AVCODEC_pkgconfig libavcodec)
set(_FFmpeg_AVFORMAT_pkgconfig libavformat)
set(_FFmpeg_AVDEVICE_pkgconfig libavdevice)
set(_FFmpeg_AVUTIL_pkgconfig libavutil)
set(_FFmpeg_AVFILTER_pkgconfig libavfilter)
set(_FFmpeg_SWSCALE_pkgconfig libswscale)
set(_FFmpeg_SWRESAMPLE_pkgconfig libswresample)

# Maps uppercase component name → primary header
set(_FFmpeg_AVCODEC_header libavcodec/avcodec.h)
set(_FFmpeg_AVFORMAT_header libavformat/avformat.h)
set(_FFmpeg_AVDEVICE_header libavdevice/avdevice.h)
set(_FFmpeg_AVUTIL_header libavutil/avutil.h)
set(_FFmpeg_AVFILTER_header libavfilter/avfilter.h)
set(_FFmpeg_SWSCALE_header libswscale/swscale.h)
set(_FFmpeg_SWRESAMPLE_header libswresample/swresample.h)

#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig and then looking up
# the libraries and include directories.
#
# component - uppercase Qt-compliant name, e.g. AVCODEC
#
macro(find_component component)
  set(_lower "${_FFmpeg_${component}_lower}")
  set(_pkgcfg "${_FFmpeg_${component}_pkgconfig}")
  set(_header "${_FFmpeg_${component}_header}")

  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_FFmpeg_${component} QUIET ${_pkgcfg})
  endif()

  find_path(
    FFmpeg_${component}_INCLUDE_DIRS
    ${_header}
    HINTS
      ${PC_FFmpeg_${component}_INCLUDEDIR}
      ${PC_FFmpeg_${component}_INCLUDE_DIRS}
      ${PC_FFmpeg_INCLUDE_DIRS}
    PATH_SUFFIXES ffmpeg
  )

  find_library(
    FFmpeg_${component}_LIBRARIES
    NAMES ${PC_FFmpeg_${component}_LIBRARIES} ${_lower}
    HINTS
      ${PC_FFmpeg_${component}_LIBDIR}
      ${PC_FFmpeg_${component}_LIBRARY_DIRS}
      ${PC_FFmpeg_LIBRARY_DIRS}
  )

  set(
    FFmpeg_${component}_DEFINITIONS
    ${PC_FFmpeg_${component}_CFLAGS_OTHER}
    CACHE STRING
    "The ${component} CFLAGS."
  )
  set(
    FFmpeg_${component}_VERSION
    ${PC_FFmpeg_${component}_VERSION}
    CACHE STRING
    "The ${component} version number."
  )

  if(FFmpeg_${component}_LIBRARIES AND FFmpeg_${component}_INCLUDE_DIRS)
    message(STATUS "  - ${component} ${FFmpeg_${component}_VERSION} found.")
    set(FFmpeg_${component}_FOUND TRUE)
  else()
    message(STATUS "  - ${component} not found.")
  endif()

  mark_as_advanced(
    FFmpeg_${component}_INCLUDE_DIRS
    FFmpeg_${component}_LIBRARIES
    FFmpeg_${component}_DEFINITIONS
    FFmpeg_${component}_VERSION
  )

  unset(_lower)
  unset(_pkgcfg)
  unset(_header)
endmacro()

message(STATUS "Searching for FFmpeg components")
find_component(AVCODEC)
find_component(AVFORMAT)
find_component(AVDEVICE)
find_component(AVUTIL)
find_component(AVFILTER)
find_component(SWSCALE)
find_component(SWRESAMPLE)

# Aggregate libraries, definitions and include dirs from requested components
set(FFmpeg_LIBRARIES "")
set(FFmpeg_DEFINITIONS "")
set(FFmpeg_INCLUDE_DIRS "")
foreach(component ${FFmpeg_FIND_COMPONENTS})
  if(FFmpeg_${component}_FOUND)
    list(APPEND FFmpeg_LIBRARIES ${FFmpeg_${component}_LIBRARIES})
    list(APPEND FFmpeg_DEFINITIONS ${FFmpeg_${component}_DEFINITIONS})
    list(APPEND FFmpeg_INCLUDE_DIRS ${FFmpeg_${component}_INCLUDE_DIRS})
  endif()
endforeach()

# Build the include path with duplicates removed.
if(FFmpeg_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES FFmpeg_INCLUDE_DIRS)
endif()

# cache the vars.
set(
  FFmpeg_INCLUDE_DIRS
  ${FFmpeg_INCLUDE_DIRS}
  CACHE STRING
  "The FFmpeg include directories."
  FORCE
)
set(
  FFmpeg_LIBRARIES
  ${FFmpeg_LIBRARIES}
  CACHE STRING
  "The FFmpeg libraries."
  FORCE
)
set(
  FFmpeg_DEFINITIONS
  ${FFmpeg_DEFINITIONS}
  CACHE STRING
  "The FFmpeg cflags."
  FORCE
)

mark_as_advanced(FFmpeg_INCLUDE_DIRS FFmpeg_LIBRARIES FFmpeg_DEFINITIONS)

# Compile the list of required vars
set(FFmpeg_REQUIRED_VARS FFmpeg_LIBRARIES FFmpeg_INCLUDE_DIRS)
foreach(component ${FFmpeg_FIND_COMPONENTS})
  list(
    APPEND
    FFmpeg_REQUIRED_VARS
    FFmpeg_${component}_LIBRARIES
    FFmpeg_${component}_INCLUDE_DIRS
  )
endforeach()

# Give a nice error message if some of the required vars are missing.
find_package_handle_standard_args(FFmpeg DEFAULT_MSG ${FFmpeg_REQUIRED_VARS})

# ---------------------------------------------------------------------------
# Create IMPORTED targets
#
# Qt6FFmpegMediaPluginImplPrivateDependencies.cmake (same content on both
# macOS and Windows) specifies exactly:
#
#   provided_targets:
#     "FFmpeg::avcodec;FFmpeg::avformat;FFmpeg::avutil;
#      FFmpeg::swresample;FFmpeg::swscale"
#
# These targets MUST exist after find_package(FFmpeg) returns, otherwise
# vcpkg's _add_executable wrapper rejects any target that transitively
# links against them.
# ---------------------------------------------------------------------------
if(FFmpeg_FOUND)
  foreach(
    component
    AVCODEC
    AVFORMAT
    AVDEVICE
    AVUTIL
    AVFILTER
    SWSCALE
    SWRESAMPLE
  )
    if(FFmpeg_${component}_FOUND)
      set(_target "FFmpeg::${_FFmpeg_${component}_lower}")
      if(NOT TARGET ${_target})
        add_library(${_target} UNKNOWN IMPORTED)
        set_target_properties(
          ${_target}
          PROPERTIES
            IMPORTED_LOCATION "${FFmpeg_${component}_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_${component}_INCLUDE_DIRS}"
            INTERFACE_COMPILE_OPTIONS "${FFmpeg_${component}_DEFINITIONS}"
        )
      endif()
      unset(_target)
    endif()
  endforeach()

  # Aggregate interface target for convenience
  if(NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    foreach(component ${FFmpeg_FIND_COMPONENTS})
      if(FFmpeg_${component}_FOUND)
        target_link_libraries(
          FFmpeg::FFmpeg
          INTERFACE "FFmpeg::${_FFmpeg_${component}_lower}"
        )
      endif()
    endforeach()
  endif()
endif()
