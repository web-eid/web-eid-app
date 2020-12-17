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
project(web-eid VERSION 0.9.3.${BUILD_NUMBER})
set(FIREFOX_UUID "{89bcad3c-5c95-4a6c-a5a5-64da932f6bf7}" CACHE STRING "Firefox Extension UUID")
set(FIREFOX_URL "https://web-eid.eu/files/firefox/web-eid-webextension-latest.xpi" CACHE STRING "Mozilla AMO URL")

find_package(Qt5 COMPONENTS Core Widgets Network Test LinguistTools REQUIRED)

add_subdirectory(lib/libelectronic-id)

set(CMAKE_AUTOMOC ON)
add_subdirectory(src/ui)
add_subdirectory(src/controller)
add_subdirectory(src/app)

enable_testing()
add_subdirectory(tests/mock-ui)
add_subdirectory(tests/tests)
