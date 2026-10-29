#[=======================================================================[.rst:
FindOnnxRuntime
---------------

Finds ONNX Runtime for running ONNX models.

On Windows only the ONNX Runtime headers and the ``onnxruntime.lib`` import
library from the buildenv are used here. The Windows ML headers and import
libraries are located by the main ``CMakeLists.txt``. Windows ML is used
framework-dependent: ``onnxruntime.dll`` and the Windows ML DLL are delay
loaded from the installed Windows App SDK runtime after the dynamic dependency
bootstrapper has been initialized.

On all other platforms ONNX Runtime is used without such wrapper
in particular:
- Ubuntu: ``libonnxruntime-dev`` package is used
- VCPKG: ``onnxruntime`` port is used

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``OnnxRuntime::OnnxRuntime``
  The ONNX Runtime library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``OnnxRuntime_FOUND``
  True if the system has the ONNX Runtime library.
``OnnxRuntime_VERSION``
  The version of the ONNX Runtime API on all platforms (derived from
  ``ORT_API_VERSION`` unless pkg-config provides it).
``OnnxRuntime_INCLUDE_DIRS``
  Include directories needed to use ONNX Runtime.
``OnnxRuntime_LIBRARIES``
  Libraries needed to link to ONNX Runtime.
``OnnxRuntime_DEFINITIONS``
  Compile definitions needed to use ONNX Runtime.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``OnnxRuntime_INCLUDE_DIR``
  The directory containing ``onnxruntime_cxx_api.h``.
``OnnxRuntime_LIBRARY``
  The path to the ONNX Runtime library.

#]=======================================================================]

# The standard "Found ..." message is replaced by an own one which tells
# whether only the import library or the complete library was found.
set(_OnnxRuntime_was_quiet ${OnnxRuntime_FIND_QUIETLY})
set(OnnxRuntime_FIND_QUIETLY TRUE)
if(WIN32)
  set(_OnnxRuntime_library_kind "import library")
else()
  set(_OnnxRuntime_library_kind "library")
endif()

if(WIN32)
  # Only the ONNX Runtime headers and import library come from the buildenv.
  # The Windows ML headers/import libraries are added by the main CMakeLists.txt
  # and all DLLs are loaded from the local Windows installation.
  find_path(
    OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS "${WINDOWSAPPSDK_ML_ROOT}"
    PATH_SUFFIXES
      include/windowsappsdk-ml/winml
      windowsappsdk-ml/winml
      onnxruntime
    DOC "ONNX Runtime include directory"
  )
  mark_as_advanced(OnnxRuntime_INCLUDE_DIR)

  find_library(
    OnnxRuntime_LIBRARY
    NAMES onnxruntime
    HINTS "${WINDOWSAPPSDK_ML_ROOT}"
    PATH_SUFFIXES lib/windowsappsdk-ml windowsappsdk-ml
    DOC "ONNX Runtime library"
  )
  mark_as_advanced(OnnxRuntime_LIBRARY)

  if(
    OnnxRuntime_INCLUDE_DIR
    AND EXISTS "${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h"
  )
    file(
      STRINGS
      "${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h"
      _OnnxRuntime_api_line
      REGEX "^#define ORT_API_VERSION [0-9]+"
    )
    if(_OnnxRuntime_api_line MATCHES "ORT_API_VERSION ([0-9]+)")
      set(OnnxRuntime_VERSION "1.${CMAKE_MATCH_1}")
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    OnnxRuntime
    REQUIRED_VARS OnnxRuntime_LIBRARY OnnxRuntime_INCLUDE_DIR
    VERSION_VAR OnnxRuntime_VERSION
  )

  if(OnnxRuntime_FOUND)
    set(OnnxRuntime_LIBRARIES "${OnnxRuntime_LIBRARY}")
    set(OnnxRuntime_INCLUDE_DIRS "${OnnxRuntime_INCLUDE_DIR}")
    set(OnnxRuntime_DEFINITIONS "")

    if(NOT TARGET OnnxRuntime::OnnxRuntime)
      add_library(OnnxRuntime::OnnxRuntime UNKNOWN IMPORTED)
      set_target_properties(
        OnnxRuntime::OnnxRuntime
        PROPERTIES
          IMPORTED_LOCATION "${OnnxRuntime_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${OnnxRuntime_INCLUDE_DIRS}"
      )
    endif()
  endif()
else()
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(PC_OnnxRuntime QUIET libonnxruntime)
  endif()

  find_path(
    OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS ${PC_OnnxRuntime_INCLUDE_DIRS}
    PATH_SUFFIXES onnxruntime
    DOC "ONNX Runtime include directory"
  )
  mark_as_advanced(OnnxRuntime_INCLUDE_DIR)

  find_library(
    OnnxRuntime_LIBRARY
    NAMES onnxruntime
    HINTS ${PC_OnnxRuntime_LIBRARY_DIRS}
    DOC "ONNX Runtime library"
  )
  mark_as_advanced(OnnxRuntime_LIBRARY)

  if(DEFINED PC_OnnxRuntime_VERSION AND NOT PC_OnnxRuntime_VERSION STREQUAL "")
    set(OnnxRuntime_VERSION "${PC_OnnxRuntime_VERSION}")
  elseif(
    OnnxRuntime_INCLUDE_DIR
    AND EXISTS "${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h"
  )
    # Header only knows the API version, which is the minor version of the release
    file(
      STRINGS
      "${OnnxRuntime_INCLUDE_DIR}/onnxruntime_c_api.h"
      _OnnxRuntime_api_line
      REGEX "^#define ORT_API_VERSION [0-9]+"
    )
    if(_OnnxRuntime_api_line MATCHES "ORT_API_VERSION ([0-9]+)")
      set(OnnxRuntime_VERSION "1.${CMAKE_MATCH_1}")
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(
    OnnxRuntime
    REQUIRED_VARS OnnxRuntime_LIBRARY OnnxRuntime_INCLUDE_DIR
    VERSION_VAR OnnxRuntime_VERSION
  )

  if(OnnxRuntime_FOUND)
    set(OnnxRuntime_LIBRARIES "${OnnxRuntime_LIBRARY}")
    set(OnnxRuntime_INCLUDE_DIRS "${OnnxRuntime_INCLUDE_DIR}")
    set(OnnxRuntime_DEFINITIONS ${PC_OnnxRuntime_CFLAGS_OTHER})

    if(NOT TARGET OnnxRuntime::OnnxRuntime)
      add_library(OnnxRuntime::OnnxRuntime UNKNOWN IMPORTED)
      set_target_properties(
        OnnxRuntime::OnnxRuntime
        PROPERTIES
          IMPORTED_LOCATION "${OnnxRuntime_LIBRARY}"
          INTERFACE_COMPILE_OPTIONS "${PC_OnnxRuntime_CFLAGS_OTHER}"
          INTERFACE_INCLUDE_DIRECTORIES "${OnnxRuntime_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()

if(OnnxRuntime_FOUND AND NOT _OnnxRuntime_was_quiet)
  if(OnnxRuntime_FIND_VERSION)
    set(
      _OnnxRuntime_version_info
      "found suitable version \"${OnnxRuntime_VERSION}\", minimum required is \"${OnnxRuntime_FIND_VERSION}\""
    )
  else()
    set(_OnnxRuntime_version_info "found version \"${OnnxRuntime_VERSION}\"")
  endif()
  message(
    STATUS
    "Found OnnxRuntime ${_OnnxRuntime_library_kind}: ${OnnxRuntime_LIBRARY} (${_OnnxRuntime_version_info})"
  )
endif()
