#[=======================================================================[.rst:
FindOnnxRuntime
---------------

Finds ONNX Runtime for running ONNX models.

On Windows the Windows App SDK Windows ML (framework-dependent) is used. It is
provided by the vcpkg overlay port ``windowsappsdk-ml``. ``onnxruntime.dll``
and the Windows ML DLL are delay loaded from the framework package after
``MddBootstrapInitialize2()``, nothing is shipped by Mixxx.

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
``OnnxRuntime_WinML_VERSION``
  Windows only: The version of the Windows ML (``windowsappsdk-ml``) package
  that provides ONNX Runtime. This is not the ONNX Runtime version.
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
``OnnxRuntime_WinML_INCLUDE_DIR``
  Windows only: The directory containing ``MddBootstrap.h``.
``OnnxRuntime_WinML_LIBRARY``
  Windows only: The path to ``Microsoft.Windows.AI.MachineLearning.lib``.
``OnnxRuntime_Bootstrap_LIBRARY``
  Windows only: The path to ``Microsoft.WindowsAppRuntime.Bootstrap.lib``.

#]=======================================================================]

if(WIN32)
  find_path(
    OnnxRuntime_WinML_INCLUDE_DIR
    NAMES MddBootstrap.h WinMLEpCatalog.h
    PATH_SUFFIXES include/windowsappsdk-ml windowsappsdk-ml
    DOC "Windows ML include directory"
  )
  mark_as_advanced(OnnxRuntime_WinML_INCLUDE_DIR)

  set(_OnnxRuntime_lib_hints "")
  if(OnnxRuntime_WinML_INCLUDE_DIR)
    # <prefix>/include/windowsappsdk-ml -> <prefix>/lib/windowsappsdk-ml
    cmake_path(
      GET
      OnnxRuntime_WinML_INCLUDE_DIR
      PARENT_PATH _OnnxRuntime_prefix
    )
    cmake_path(GET _OnnxRuntime_prefix PARENT_PATH _OnnxRuntime_prefix)
    set(_OnnxRuntime_lib_hints "${_OnnxRuntime_prefix}/lib/windowsappsdk-ml")
  endif()

  find_path(
    OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS "${OnnxRuntime_WinML_INCLUDE_DIR}/winml"
    DOC "ONNX Runtime include directory"
  )
  mark_as_advanced(OnnxRuntime_INCLUDE_DIR)

  find_library(
    OnnxRuntime_LIBRARY
    NAMES onnxruntime
    HINTS ${_OnnxRuntime_lib_hints}
    NO_DEFAULT_PATH
    DOC "ONNX Runtime library"
  )
  mark_as_advanced(OnnxRuntime_LIBRARY)

  find_library(
    OnnxRuntime_WinML_LIBRARY
    NAMES Microsoft.Windows.AI.MachineLearning
    HINTS ${_OnnxRuntime_lib_hints}
    NO_DEFAULT_PATH
    DOC "Windows ML library"
  )
  mark_as_advanced(OnnxRuntime_WinML_LIBRARY)

  find_library(
    OnnxRuntime_Bootstrap_LIBRARY
    NAMES Microsoft.WindowsAppRuntime.Bootstrap
    HINTS ${_OnnxRuntime_lib_hints}
    NO_DEFAULT_PATH
    DOC "Windows App Runtime bootstrap library"
  )
  mark_as_advanced(OnnxRuntime_Bootstrap_LIBRARY)

  # The version of the windowsappsdk-ml port is recorded in its SPDX file
  if(OnnxRuntime_WinML_INCLUDE_DIR)
    set(
      _OnnxRuntime_spdx
      "${_OnnxRuntime_prefix}/share/windowsappsdk-ml/vcpkg.spdx.json"
    )
    if(EXISTS "${_OnnxRuntime_spdx}")
      file(READ "${_OnnxRuntime_spdx}" _OnnxRuntime_spdx_content)
      string(
        REGEX MATCH
        "\"versionInfo\": *\"([0-9][^\"]*)\""
        _OnnxRuntime_version_match
        "${_OnnxRuntime_spdx_content}"
      )
      if(_OnnxRuntime_version_match)
        set(OnnxRuntime_WinML_VERSION "${CMAKE_MATCH_1}")
      endif()
    endif()
  endif()

  # The ONNX Runtime version, same detection as on the other platforms
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
    REQUIRED_VARS
      OnnxRuntime_LIBRARY
      OnnxRuntime_INCLUDE_DIR
      OnnxRuntime_WinML_INCLUDE_DIR
      OnnxRuntime_WinML_LIBRARY
    VERSION_VAR OnnxRuntime_VERSION
  )

  if(OnnxRuntime_FOUND)
    message(STATUS "Windows ML package version: ${OnnxRuntime_WinML_VERSION}")
    set(
      OnnxRuntime_LIBRARIES
      "${OnnxRuntime_LIBRARY}"
      "${OnnxRuntime_WinML_LIBRARY}"
      "${OnnxRuntime_Bootstrap_LIBRARY}"
    )
    set(
      OnnxRuntime_INCLUDE_DIRS
      "${OnnxRuntime_INCLUDE_DIR}"
      "${OnnxRuntime_WinML_INCLUDE_DIR}"
    )
    set(OnnxRuntime_DEFINITIONS "")

    if(NOT TARGET OnnxRuntime::OnnxRuntime)
      add_library(OnnxRuntime::OnnxRuntime INTERFACE IMPORTED)
      set_target_properties(
        OnnxRuntime::OnnxRuntime
        PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${OnnxRuntime_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES
            "${OnnxRuntime_LIBRARIES};delayimp;-DELAYLOAD:onnxruntime.dll;-DELAYLOAD:Microsoft.Windows.AI.MachineLearning.dll"
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
