project(SDSService)

set(ASIO_INCLUDE_DIR "NOTFOUND" CACHE PATH "Path to ASIO include directory - note do not use boost.asio")

set(CMAKE_CXX_STANDARD 20)

if (NOT ASIO_INCLUDE_DIR)
	message(FATAL_ERROR "ASIO Include Directory not specificed")
endif()

add_executable(SDSService service.cpp ServiceUtils.cpp Service.h ServiceMain.h ServiceUtils.h SDSService.h)
target_include_directories(SDSService PRIVATE ${ASIO_INCLUDE_DIR})
set_property(TARGET SDSService PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
set_property(TARGET SDSService PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:CONSOLE")
