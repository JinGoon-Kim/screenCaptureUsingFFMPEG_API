cmake_minimum_required(VERSION 3.20)

PROJECT ("ScreenCaptureUsingFfmpegApi")

###########################################
# - variables -----------------------------
if( DEFINED FFMPEG_DIR )
    message( STATUS " FFMPEG_DIR IS defined. ${FFMPEG_DIR}" )
else()
    message( FATAL_ERROR " FFMPEG_DIR not defined." )
endif()

###########################################
# - build type ----------------------------
# Set a default build type if none was specified
IF(NOT MSVC)
  set(DEFAULT_BUILD_TYPE "Release")

  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' since none specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
      "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()
endif()
###########################################
# - compiler options ----------------------
set(CMAKE_CXX_STANDARD 17)
add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

###########################################
# set output path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE})

# set include path
include_directories(
    ${FFMPEG_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

# set library path
link_directories(
    ${FFMPEG_DIR}/lib
)

###########################################
# make executable
add_executable(${CMAKE_PROJECT_NAME}
    src/main.cpp
	src/screen_recorder.cpp
)

# set dependency
target_link_libraries(${CMAKE_PROJECT_NAME}
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    postproc
    swresample
    swscale
)