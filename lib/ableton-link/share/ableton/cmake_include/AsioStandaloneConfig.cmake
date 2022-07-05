add_library(AsioStandalone::AsioStandalone IMPORTED INTERFACE)
if (LINK_BUILD_ASIO MATCHES "ON") 

  find_package(asio REQUIRED)
  if(asio_FOUND)
    message(STATUS "Dependency asio found in ${asio_DIR}")   
    set_property(TARGET AsioStandalone::AsioStandalone APPEND PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES
      ${asio_DIR}/../../include
    )
  endif()
endif()
