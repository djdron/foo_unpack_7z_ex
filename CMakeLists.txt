cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)

project(foo_unpack_7z_ex)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if("${FOOBAR_SDK_ROOT}" STREQUAL "")
message(FATAL_ERROR "Unable to find the foobar2000 SDK. Please set FOOBAR_SDK_ROOT to the root directory containing foobar2000 SDK. Get it here: http://www.foobar2000.org/SDK")
elseif(NOT EXISTS "${FOOBAR_SDK_ROOT}/foobar2000/SDK/foobar2000.h")
message(FATAL_ERROR "Unable to find the foobar2000 SDK in ${FOOBAR_SDK_ROOT}")
else()
include_directories("${FOOBAR_SDK_ROOT}/foobar2000")
endif()

include_directories(".")
add_definitions(-DEXTRACT_ONLY -D_7ZIP_ST)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4244 /wd4800 /wd4996")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4090")

file(GLOB SRCS_C_7Z "7z/C/*.c")
file(GLOB SRCS_CH_7Z "7z/C/*.h")

file(GLOB_RECURSE SRCS_CXX_7Z "7z/CPP/*.cpp")
file(GLOB_RECURSE SRCS_H_7Z "7z/CPP/*.h")

file(GLOB SRCS_CXX "src/*.cpp")
file(GLOB SRCS_H "src/*.h")

#setup source groups (folders in solution)
source_group("7z\\c" FILES ${SRCS_C_7Z} ${SRCS_CH_7Z})

file(GLOB SRCS_CXX_7Z_COMMON "7z/CPP/Common/*.cpp")
file(GLOB SRCS_H_7Z_COMMON "7z/CPP/Common/*.h")
source_group("7z\\cpp\\common" FILES ${SRCS_CXX_7Z_COMMON} ${SRCS_H_7Z_COMMON})

file(GLOB SRCS_CXX_7Z_WINDOWS "7z/CPP/Windows/*.cpp")
file(GLOB SRCS_H_7Z_WINDOWS "7z/CPP/Windows/*.h")
source_group("7z\\cpp\\windows" FILES ${SRCS_CXX_7Z_WINDOWS} ${SRCS_H_7Z_WINDOWS})

file(GLOB SRCS_H_7Z_7ZIP "7z/CPP/7zip/*.h")
source_group("7z\\cpp\\7zip" FILES ${SRCS_H_7Z_7ZIP})

file(GLOB SRCS_CXX_7Z_7ZIP_COMMON "7z/CPP/7zip/Common/*.cpp")
file(GLOB SRCS_H_7Z_7ZIP_COMMON "7z/CPP/7zip/Common/*.h")
source_group("7z\\cpp\\7zip\\common" FILES ${SRCS_CXX_7Z_7ZIP_COMMON} ${SRCS_H_7Z_7ZIP_COMMON})

file(GLOB SRCS_CXX_7Z_7ZIP_COMPRESS "7z/CPP/7zip/Compress/*.cpp")
file(GLOB SRCS_H_7Z_7ZIP_COMPRESS "7z/CPP/7zip/Compress/*.h")
source_group("7z\\cpp\\7zip\\compress" FILES ${SRCS_CXX_7Z_7ZIP_COMPRESS} ${SRCS_H_7Z_7ZIP_COMPRESS})

file(GLOB SRCS_CXX_7Z_7ZIP_ARCHIVE "7z/CPP/7zip/Archive/*.cpp")
file(GLOB SRCS_H_7Z_7ZIP_ARCHIVE "7z/CPP/7zip/Archive/*.h")
source_group("7z\\cpp\\7zip\\archive" FILES ${SRCS_CXX_7Z_7ZIP_ARCHIVE} ${SRCS_H_7Z_7ZIP_ARCHIVE})

file(GLOB SRCS_CXX_7Z_7ZIP_ARCHIVE_7Z "7z/CPP/7zip/Archive/7z/*.cpp")
file(GLOB SRCS_H_7Z_7ZIP_ARCHIVE_7Z "7z/CPP/7zip/Archive/7z/*.h")
source_group("7z\\cpp\\7zip\\archive\\7z" FILES ${SRCS_CXX_7Z_7ZIP_ARCHIVE_7Z} ${SRCS_H_7Z_7ZIP_ARCHIVE_7Z})

file(GLOB SRCS_CXX_7Z_7ZIP_ARCHIVE_COMMON "7z/CPP/7zip/Archive/common/*.cpp")
file(GLOB SRCS_H_7Z_7ZIP_ARCHIVE_COMMON "7z/CPP/7zip/Archive/common/*.h")
source_group("7z\\cpp\\7zip\\archive\\common" FILES ${SRCS_CXX_7Z_7ZIP_ARCHIVE_COMMON} ${SRCS_H_7Z_7ZIP_ARCHIVE_COMMON})

source_group("src" FILES ${SRCS_CXX} ${SRCS_H})

set(FOOBAR_SDK_LIBRARIES
	optimized "${FOOBAR_SDK_ROOT}/foobar2000/SDK/Release/foobar2000_SDK.lib"
	optimized "${FOOBAR_SDK_ROOT}/foobar2000/foobar2000_component_client/Release/foobar2000_component_client.lib"
	optimized "${FOOBAR_SDK_ROOT}/foobar2000/helpers/Release/foobar2000_sdk_helpers.lib"
	optimized "${FOOBAR_SDK_ROOT}/foobar2000/shared/shared.lib"
	optimized "${FOOBAR_SDK_ROOT}/pfc/Release/pfc.lib"
	debug "${FOOBAR_SDK_ROOT}/foobar2000/SDK/Debug/foobar2000_SDK.lib"
	debug "${FOOBAR_SDK_ROOT}/foobar2000/foobar2000_component_client/Debug/foobar2000_component_client.lib"
	debug "${FOOBAR_SDK_ROOT}/foobar2000/helpers/Debug/foobar2000_sdk_helpers.lib"
	debug "${FOOBAR_SDK_ROOT}/foobar2000/shared/shared.lib"
	debug "${FOOBAR_SDK_ROOT}/pfc/Debug/pfc.lib"
)

add_library(foo_unpack_7z_ex MODULE ${SRCS_CXX} ${SRCS_H} ${SRCS_C_7Z} ${SRCS_CH_7Z} ${SRCS_CXX_7Z} ${SRCS_H_7Z})
target_link_libraries(foo_unpack_7z_ex ${FOOBAR_SDK_LIBRARIES})
