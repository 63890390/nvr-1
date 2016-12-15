project(nvr)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

set(TARGET_DIR "bin")
set(SOURCE_FILES main.c jsmn.c jsmn.h utils.c utils.h nvr.c nvr.h config.c config.h ini.c ini.h types.h log.h log.c)

add_executable(nvr ${SOURCE_FILES})

find_package(FFmpeg REQUIRED)
if (FFMPEG_FOUND)
    include_directories(${FFMPEG_INCLUDE_DIR})
    target_link_libraries(nvr ${FFMPEG_LIBAVCODEC})
    target_link_libraries(nvr ${FFMPEG_LIBAVFORMAT})
    target_link_libraries(nvr ${FFMPEG_LIBAVUTIL})
else (FFMPEG_FOUND)
    message(FATAL_ERROR "FFmpeg libraries not found!")
endif (FFMPEG_FOUND)

target_link_libraries(nvr "-lm")
target_link_libraries(nvr "-lpthread")
