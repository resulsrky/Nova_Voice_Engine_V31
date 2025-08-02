# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(NovaEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find Core Libraries ---
# FFmpeg - use pkg-config instead of find_package
find_package(PkgConfig REQUIRED)

# FFmpeg kütüphanelerini kontrol et - ŞİMDİLİK DEVRE DIŞI
# pkg_check_modules(FFMPEG libavcodec libavformat libavutil libswscale)
# if(NOT FFMPEG_FOUND)
#     message(WARNING "FFmpeg kütüphaneleri bulunamadı. Video encoding devre dışı.")
#     set(FFMPEG_AVAILABLE FALSE)
# else()
#     set(FFMPEG_AVAILABLE TRUE)
#     message(STATUS "FFmpeg kütüphaneleri bulundu")
# endif()

# FFmpeg'i şimdilik devre dışı bırak
set(FFMPEG_AVAILABLE FALSE)
message(STATUS "FFmpeg şimdilik devre dışı - sadece JPEG encoding kullanılacak")

# OpenCV kontrol et
find_package(OpenCV)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV bulunamadı. Lütfen OpenCV kurun: sudo apt install libopencv-dev")
else()
    message(STATUS "OpenCV bulundu: ${OpenCV_VERSION}")
endif()

# --- Jerasure & GF-Complete (Reed-Solomon) ---
# Geçici olarak devre dışı - Jerasure kütüphanesi eksik
# OPTION 1: Add as subdirectory (Recommended)
# Clone jerasure and gf-complete into a 'libs' folder
# add_subdirectory(libs/gf-complete)
# add_subdirectory(libs/jerasure)
# include_directories(libs/gf-complete/include libs/jerasure/include)
# link_libraries(Jerasure gf_complete)

# OPTION 2: Find system-installed libraries
# find_library(JERASURE_LIBRARY NAMES Jerasure jerasure)
# find_library(GF_COMPLETE_LIBRARY NAMES gf_complete)
# find_path(JERASURE_INCLUDE_DIR jerasure.h)
# find_path(GF_COMPLETE_INCLUDE_DIR gf_complete.h)

# if(NOT JERASURE_LIBRARY OR NOT GF_COMPLETE_LIBRARY)
#     message(FATAL_ERROR "Jerasure or GF-Complete not found. Please install or add as subdirectory.")
# endif()

# include_directories(${JERASURE_INCLUDE_DIR} ${GF_COMPLETE_INCLUDE_DIR})
include_directories(src)
set(CUSTOM_LIBS "")

# --- Add Your Executable ---
add_executable(nova_engine 
    src/core/main.cpp
    src/core/engine.cpp
    src/media/slicer.cpp
    src/media/erasure_coder.cpp
    src/network/scheduler.cpp
    src/network/sender_receiver.cpp
    src/network/path_monitor.cpp
    src/transport/smart_collector.cpp
)

# FFmpeg varsa ffmpeg_encoder.cpp ekle
if(FFMPEG_AVAILABLE)
    target_sources(nova_engine PRIVATE src/media/ffmpeg_encoder.cpp)
    target_compile_definitions(nova_engine PRIVATE FFMPEG_AVAILABLE)
endif()

# --- Add Friend's Executable ---
add_executable(nova_engine_friend 
    src/core/main_friend.cpp
    src/core/engine.cpp
    src/media/slicer.cpp
    src/media/erasure_coder.cpp
    src/network/scheduler.cpp
    src/network/sender_receiver.cpp
    src/network/path_monitor.cpp
    src/transport/smart_collector.cpp
)

# FFmpeg varsa ffmpeg_encoder.cpp ekle
if(FFMPEG_AVAILABLE)
    target_sources(nova_engine_friend PRIVATE src/media/ffmpeg_encoder.cpp)
    target_compile_definitions(nova_engine_friend PRIVATE FFMPEG_AVAILABLE)
endif()

# --- Add UDP Test Applications ---
add_executable(udp_test src/core/udp_test.cpp)
add_executable(udp_test_friend src/core/udp_test_friend.cpp)
add_executable(udp_chat src/core/udp_chat.cpp)
add_executable(video_chat src/core/main.cpp)

# --- Link All Libraries ---
target_link_libraries(nova_engine
    PRIVATE
    ${OpenCV_LIBS}
    ${CUSTOM_LIBS}
    pthread
)

target_link_libraries(nova_engine_friend
    PRIVATE
    ${OpenCV_LIBS}
    ${CUSTOM_LIBS}
    pthread
)

# FFmpeg varsa linkle - DÜZELTME: Doğru kütüphane isimleri
if(FFMPEG_AVAILABLE)
    target_link_libraries(nova_engine PRIVATE 
        avcodec avformat avutil swscale
    )
    target_link_libraries(nova_engine_friend PRIVATE 
        avcodec avformat avutil swscale
    )
    target_include_directories(nova_engine PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_include_directories(nova_engine_friend PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_compile_options(nova_engine PRIVATE ${FFMPEG_CFLAGS_OTHER})
    target_compile_options(nova_engine_friend PRIVATE ${FFMPEG_CFLAGS_OTHER})
    target_link_directories(nova_engine PRIVATE ${FFMPEG_LIBRARY_DIRS})
    target_link_directories(nova_engine_friend PRIVATE ${FFMPEG_LIBRARY_DIRS})
endif()

target_link_libraries(udp_test PRIVATE pthread)
target_link_libraries(udp_test_friend PRIVATE pthread)
target_link_libraries(video_chat PRIVATE pthread ${OpenCV_LIBS})
target_link_libraries(udp_chat PRIVATE pthread)

# --- Compiler Flags for Performance ---
target_compile_options(nova_engine PRIVATE -O3 -DNDEBUG)
target_compile_options(nova_engine_friend PRIVATE -O3 -DNDEBUG)