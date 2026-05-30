#[=======================================================================[.rst:
FindFdkAac
----------

Finds the Fraunhofer FDK AAC library (libfdk-aac).

FDK-AAC is used in two distinct ways in Mixxx:

1. **Runtime dynamic loading** (``EncoderFdkAac`` via ``QLibrary``) — the
    shared library is bundled into the macOS/Windows packages but never
    linked at build time.

2. **Transitive static dependency of FFmpeg** — when FFmpeg's
    ``libavcodec.a`` is built with ``--enable-libfdk-aac`` the FDK-AAC
    symbols must be satisfied at final link time.  ``FindFFmpeg.cmake``
    calls this module and attaches ``FdkAac::FdkAac`` as a transitive
    dependency of the ``FFmpeg::avcodec`` imported target whenever
    ``libavcodec`` is a static library.

Imported Targets
^^^^^^^^^^^^^^^^

``FdkAac::FdkAac``
  The FDK-AAC library.

Result Variables
^^^^^^^^^^^^^^^^

``FdkAac_FOUND``
  True if the system has the FDK-AAC library.
``FdkAac_INCLUDE_DIRS``
  Include directories needed to use FDK-AAC (may be empty when headers
  are absent — only required when compiling against FDK-AAC directly).
``FdkAac_DLL``
  (Windows only) Full path to ``fdk-aac.dll``, suitable for install().

Cache Variables
^^^^^^^^^^^^^^^

``FdkAac_INCLUDE_DIR``
  The directory containing ``fdk-aac/aacenc_lib.h`` (optional).
``FdkAac_LIBRARY``
  The FDK-AAC link library (``.dylib`` / ``.so`` / ``.a`` / ``.lib``).
``FdkAac_DLL``
  (Windows only) Path to ``fdk-aac.dll``.

#]=======================================================================]

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_FdkAac QUIET fdk-aac)
endif()

find_path(
  FdkAac_INCLUDE_DIR
  NAMES fdk-aac/aacenc_lib.h
  HINTS ${PC_FdkAac_INCLUDE_DIRS}
  DOC "FDK-AAC include directory (optional)"
)
mark_as_advanced(FdkAac_INCLUDE_DIR)

find_library(
  FdkAac_LIBRARY
  NAMES fdk-aac libfdk-aac
  HINTS ${PC_FdkAac_LIBRARY_DIRS}
  DOC "FDK-AAC link library"
)
mark_as_advanced(FdkAac_LIBRARY)

# On Windows find_library() returns the .lib import library.
# Locate the companion .dll separately so callers can install it.
if(WIN32)
  find_file(
    FdkAac_DLL
    NAMES fdk-aac.dll
    HINTS ${PC_FdkAac_LIBRARY_DIRS}
    PATH_SUFFIXES ${CMAKE_INSTALL_BINDIR} bin
    DOC "FDK-AAC DLL (Windows runtime)"
  )
  mark_as_advanced(FdkAac_DLL)
endif()

include(FindPackageHandleStandardArgs)
# Only the library is required. Headers are optional: on Android the library
# is a transitive static dep of libavcodec.a and no Mixxx code includes
# FDK-AAC headers directly, so the include dir may legitimately be absent.
find_package_handle_standard_args(
  FdkAac
  REQUIRED_VARS FdkAac_LIBRARY
  VERSION_VAR PC_FdkAac_VERSION
)

if(FdkAac_FOUND AND NOT TARGET FdkAac::FdkAac)
  # FdkAac_INCLUDE_DIRS: empty string when headers not present — callers must
  # not assume non-empty (Android links transitively; no headers needed).
  set(FdkAac_INCLUDE_DIRS "${FdkAac_INCLUDE_DIR}")

  add_library(FdkAac::FdkAac UNKNOWN IMPORTED)
  set_target_properties(
    FdkAac::FdkAac
    PROPERTIES IMPORTED_LOCATION "${FdkAac_LIBRARY}"
  )
  if(FdkAac_INCLUDE_DIR)
    set_target_properties(
      FdkAac::FdkAac
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FdkAac_INCLUDE_DIR}"
    )
  endif()
  if(WIN32 AND FdkAac_DLL)
    # Expose the DLL as the runtime artifact; the .lib is the link artifact.
    set_target_properties(
      FdkAac::FdkAac
      PROPERTIES
        IMPORTED_LOCATION "${FdkAac_DLL}"
        IMPORTED_IMPLIB "${FdkAac_LIBRARY}"
    )
  endif()
endif()
