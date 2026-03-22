cmake_minimum_required(VERSION 3.18)
project(canvasos_engine)

# 프로젝트 내 native/build 경로 (v2 C 엔진 소스)
# 절대 경로 폴백: 디바이스에서 직접 빌드 시
set(NATIVE_ROOT ${CMAKE_SOURCE_DIR}/../../../native/build)
if(NOT EXISTS ${NATIVE_ROOT}/src)
  set(NATIVE_ROOT "/storage/emulated/0/Download/KakaoTalk/Canvas-OS-App-v1.1.0/native/build")
endif()
set(CPP_ROOT ${CMAKE_SOURCE_DIR}/../cpp)

file(GLOB ENGINE_C_SOURCES
    ${NATIVE_ROOT}/src/*.c
    ${NATIVE_ROOT}/src/tervas/*.c
)

list(FILTER ENGINE_C_SOURCES EXCLUDE REGEX ".*/canvasos_launcher\\.c$")

set(TURBO_SOURCES
    ${CPP_ROOT}/CanvasOSEngineTurbo.cpp
    ${CPP_ROOT}/QueryDispatcher.cpp
    ${CPP_ROOT}/OnLoad.cpp
)

add_library(canvasos_engine SHARED
    ${ENGINE_C_SOURCES}
    ${TURBO_SOURCES}
)

target_include_directories(canvasos_engine PRIVATE
    ${NATIVE_ROOT}/include
    ${NATIVE_ROOT}/src
    ${CPP_ROOT}
)

target_compile_options(canvasos_engine PRIVATE
    -DANDROID
    -DCANVASOS_TURBO_MODULE
    -fvisibility=hidden
    -O2
    -Wall
)

find_package(fbjni REQUIRED CONFIG)
find_package(ReactAndroid REQUIRED CONFIG)

target_link_libraries(canvasos_engine
    fbjni::fbjni
    ReactAndroid::jsi
    ReactAndroid::reactnative
    android
    log
)
