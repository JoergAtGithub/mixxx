if (NEED_CATCH2 MATCHES "ON") 
  add_library(Catch::Catch IMPORTED INTERFACE)

  find_package(Catch2 CONFIG REQUIRED)
  if(Catch2_FOUND)
    message(STATUS "Dependency Catch2 found in ${Catch2_DIR}")  
    set_property(TARGET Catch::Catch APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES
      ${Catch2_DIR}/../../include
    )
  endif()
endif()
