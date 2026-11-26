# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.22)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/libelectronic-id/README.md")
    message(FATAL_ERROR "libelectronic-id submodule directory empty, did you 'git clone --recursive'?")
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET 14.4 CACHE STRING "Minimum macOS version (Apple only)")

if($ENV{BUILD_NUMBER})
    set(BUILD_NUMBER $ENV{BUILD_NUMBER})
elseif($ENV{CI_PIPELINE_IID})
    set(BUILD_NUMBER $ENV{CI_PIPELINE_IID})
else()
    set(BUILD_NUMBER 0)
endif()
project(web-eid VERSION 2.11.0.${BUILD_NUMBER}
    DESCRIPTION "Web eID browser extension helper application"
    HOMEPAGE_URL https://github.com/web-eid/web-eid-app
)

set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION_TWEAK})
set(SEMVER_VERSION_STRING "${MACOSX_BUNDLE_SHORT_VERSION_STRING}+${PROJECT_VERSION_TWEAK}")
set(CMAKE_POSITION_INDEPENDENT_CODE YES)
set(CPACK_GENERATOR RPM)
set(CPACK_PACKAGE_CONTACT "RIA <info@ria.ee>")
set(CPACK_PACKAGE_VENDOR RIA)
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_PACKAGE_DESCRIPTION "The Web eID application and browser extensions enable usage of electronic ID smart \
cards for secure authentication and digital signing of documents on the web using public-key cryptography.")
set(CPACK_RPM_NATIVE_PACKAGE_NAME web-eid-native)
set(CPACK_RPM_NATIVE_PACKAGE_SUMMARY ${CMAKE_PROJECT_DESCRIPTION})
set(CPACK_RPM_NATIVE_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}\n\n\
Native messaging host for Web eID Chrome and Firefox extensions that performs cryptographic digital signing and \
authentication operations with electronic ID smart cards. Also works standalone without extensions in command-line mode.")
set(CPACK_RPM_NATIVE_PACKAGE_REQUIRES pcsc-lite)
set(CPACK_RPM_FIREFOX_PACKAGE_NAME web-eid-firefox)
set(CPACK_RPM_FIREFOX_PACKAGE_SUMMARY "Web eID browser extension for Firefox")
set(CPACK_RPM_FIREFOX_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}\n\n\
The Web eID extension for Firefox communicates with the Web eID native application using Native messaging.")
set(CPACK_RPM_FIREFOX_PACKAGE_REQUIRES "web-eid-native = ${CPACK_PACKAGE_VERSION}, mozilla-filesystem")
set(CPACK_RPM_CHROME_PACKAGE_NAME web-eid-chrome)
set(CPACK_RPM_CHROME_PACKAGE_SUMMARY "Web eID browser extension for Chrome")
set(CPACK_RPM_CHROME_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}\n\n\
The Web eID extension for Chrome communicates with the Web eID native application using Native messaging.")
set(CPACK_RPM_CHROME_PACKAGE_REQUIRES "web-eid-native = ${CPACK_PACKAGE_VERSION}")
set(CPACK_RPM_CHROME_PACKAGE_ARCHITECTURE noarch)
set(CPACK_RPM_META_PACKAGE_NAME web-eid)
set(CPACK_RPM_META_PACKAGE_SUMMARY "Web eID application suite (metapackage)")
set(CPACK_RPM_META_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}\n\n\
This metapackage installs the Web eID native application and the browser extensions.")
set(CPACK_RPM_META_PACKAGE_REQUIRES "web-eid-native = ${CPACK_PACKAGE_VERSION}")
set(CPACK_RPM_META_PACKAGE_RECOMMENDS "web-eid-firefox, web-eid-chrome")
set(CPACK_RPM_META_PACKAGE_ARCHITECTURE noarch)
set(CPACK_COMPONENTS_ALL meta native firefox chrome)
# Owned by filesystem, hicolor-icon-theme, mozilla-filesystem and browser packages; owning them
# too causes RPM directory conflicts (https://github.com/web-eid/web-eid-app/pull/173)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /etc/chromium /etc/opt /etc/opt/chrome
    /usr/lib/mozilla /usr/lib/mozilla/native-messaging-hosts
    /usr/lib64/mozilla /usr/lib64/mozilla/native-messaging-hosts
    /usr/share/applications /usr/share/chromium /usr/share/google-chrome
    /usr/share/icons /usr/share/icons/hicolor
    /usr/share/mozilla /usr/share/mozilla/extensions
    /usr/share/mozilla/extensions/{ec8030f7-c20a-464f-9b0e-13a3a9e97384})
foreach(RES 16 32 128 256 512)
    list(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/share/icons/hicolor/${RES}x${RES} /usr/share/icons/hicolor/${RES}x${RES}/apps)
endforeach()
include(CPack)
cpack_add_component(meta DISPLAY_NAME "web-eid" DEPENDS native)
cpack_add_component(native DISPLAY_NAME "web-eid-native")
cpack_add_component(firefox DISPLAY_NAME "web-eid-firefox" DEPENDS native)
cpack_add_component(chrome DISPLAY_NAME "web-eid-chrome" DEPENDS native)

set(FIREFOX_UUID "{e68418bc-f2b0-4459-a9ea-3e72b6751b07}" CACHE STRING "Firefox Extension UUID")
set(FIREFOX_URL "https://addons.mozilla.org/firefox/downloads/latest/web-eid-webextension/latest.xpi" CACHE STRING "Mozilla AMO URL")

set(SIGNCERT "" CACHE STRING "Common name of certificate to used sign binaries, empty skips signing  (Windows/macOS)")
set(SAFARI_PROVISIONPROFILE "" CACHE STRING "Provision profile to include in application (macOS)")

find_package(Qt6 6.2.0 REQUIRED COMPONENTS Core Widgets Network Test LinguistTools SvgWidgets)

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
