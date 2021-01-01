cmake_minimum_required(VERSION 3.8.0)
if(POLICY CMP0092)
  cmake_policy(SET CMP0092 NEW)
endif()

if($ENV{BUILD_NUMBER})
    set(BUILD_NUMBER $ENV{BUILD_NUMBER})
elseif($ENV{CI_PIPELINE_IID})
    set(BUILD_NUMBER $ENV{CI_PIPELINE_IID})
else()
    set(BUILD_NUMBER 0)
endif()
project(web-eid VERSION 1.0.0.${BUILD_NUMBER})

set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION_TWEAK})
set(SEMVER_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}+${PROJECT_VERSION_TWEAK}")

set(FIREFOX_UUID "{e68418bc-f2b0-4459-a9ea-3e72b6751b07}" CACHE STRING "Firefox Extension UUID")
set(FIREFOX_URL "https://addons.mozilla.org/firefox/downloads/latest/web-eid-webextension/latest.xpi" CACHE STRING "Mozilla AMO URL")

set(SIGNCERT "" CACHE STRING "Common name of certificate to used sign binaries, empty skip signing")
set(CROSSSIGNCERT "" CACHE STRING "Common name of certificate to used cross sign binaries, empty skip signing")

find_package(Qt5 COMPONENTS Core Widgets Network Test LinguistTools REQUIRED)

add_subdirectory(lib/libelectronic-id)

set(CMAKE_AUTOMOC ON)
add_subdirectory(src/ui)
add_subdirectory(src/controller)
add_subdirectory(src/app)
if(APPLE)
    add_subdirectory(src/mac)
endif()

enable_testing()
add_subdirectory(tests/mock-ui)
add_subdirectory(tests/tests)
