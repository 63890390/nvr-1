project(nvr)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

set(TARGET_DIR "bin")
set(SOURCE_FILES main.c jsmn.c jsmn.h utils.c utils.h nvr.c nvr.h config.c config.h ini.c ini.h types.h)

add_executable(nvr ${SOURCE_FILES})

find_package(FFmpeg REQUIRED)
# FFMPEG_FOUND - system has ffmpeg or libav
# FFMPEG_INCLUDE_DIR - the ffmpeg include directory
# FFMPEG_LIBRARIES
# FFMPEG_LIBAVCODEC
# FFMPEG_LIBAVFORMAT
# FFMPEG_LIBAVUTIL
if (FFMPEG_FOUND)
    message("Found FFMPEG/LibAV libraries")
    include_directories(${FFMPEG_INCLUDE_DIR})
    target_link_libraries(nvr ${FFMPEG_LIBRARIES})
else (FFMPEG_FOUND)
    message("Can't find libavcodec, libavformat or libavutil. Add them!")
endif (FFMPEG_FOUND)

target_link_libraries(nvr "-lm")
target_link_libraries(nvr "-lavresample")
target_link_libraries(nvr "-lpthread")
