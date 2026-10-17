# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

# SBOM generation using DEMCON/cmake-sbom (SPDX 2.3, install-time)
# Run: cmake --install <build-dir>/sbom

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/cmake-sbom/cmake")
include(sbom)

execute_process(
    COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/libelectronic-id"
    OUTPUT_VARIABLE ELECTRONIC_ID_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
string(REGEX REPLACE "^v" "" ELECTRONIC_ID_VERSION "${ELECTRONIC_ID_VERSION}")

sbom_generate(
    OUTPUT "${CMAKE_BINARY_DIR}/web-eid-${PROJECT_VERSION}.spdx"
    LICENSE "MIT"
    SUPPLIER "Estonian Information System Authority"
    SUPPLIER_URL https://www.ria.ee
    DOWNLOAD_URL https://github.com/web-eid/web-eid-app
    VERSION "${PROJECT_VERSION}"
)

set(_sbom_reset "${CMAKE_BINARY_DIR}/sbom/sbom-reset.cmake")
file(WRITE "${_sbom_reset}"
    "file(WRITE \"${CMAKE_BINARY_DIR}/sbom/sbom.spdx.in\" \"\")\n"
    "file(READ \"${CMAKE_BINARY_DIR}/SPDXRef-DOCUMENT.spdx.in\" _doc)\n"
    "file(APPEND \"${CMAKE_BINARY_DIR}/sbom/sbom.spdx.in\" \"\${_doc}\")\n"
    "set(SBOM_VERIFICATION_CODES \"\")\n"
)
file(APPEND "${CMAKE_BINARY_DIR}/sbom/CMakeLists.txt"
    "install(SCRIPT \"${_sbom_reset}\")\n"
)

set(_app_spdxid "SPDXRef-Package-${PROJECT_NAME} DEPENDS_ON @SBOM_LAST_SPDXID@")
if(APPLE)
    sbom_add(PACKAGE web-eid-safari
        VERSION "${PROJECT_VERSION}"
        SUPPLIER "Organization: Estonian Information System Authority"
        DOWNLOAD_LOCATION https://github.com/web-eid/web-eid-app
        LICENSE "MIT"
        EXTREF "cpe:2.3:a:web-eid:web-eid:${PROJECT_VERSION}:*:*:*:*:*:*:*"
        RELATIONSHIP "@SBOM_LAST_SPDXID@ VARIANT_OF SPDXRef-Package-${PROJECT_NAME}"
    )
    set(_app_spdxid "${_app_spdxid}\nRelationship: ${SBOM_LAST_SPDXID} DEPENDS_ON @SBOM_LAST_SPDXID@")
    file(READ "${CMAKE_SOURCE_DIR}/src/mac/js/package.json" _webext_json)
    string(JSON WEBEXT_VERSION GET "${_webext_json}" "version")
    sbom_add(PACKAGE web-eid-webextension
        VERSION "${WEBEXT_VERSION}"
        SUPPLIER "Organization: Estonian Information System Authority"
        DOWNLOAD_LOCATION https://github.com/web-eid/web-eid-webextension
        LICENSE "MIT"
        RELATIONSHIP "${SBOM_LAST_SPDXID} DEPENDS_ON @SBOM_LAST_SPDXID@"
    )
    if(NPM_EXECUTABLE)
        execute_process(
            COMMAND "${NPM_EXECUTABLE}" --version
            OUTPUT_VARIABLE NPM_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        string(REGEX REPLACE "^v" "" NPM_VERSION "${NPM_VERSION}")
    endif()
    if(NPM_VERSION)
        sbom_add(PACKAGE npm
            VERSION "${NPM_VERSION}"
            SUPPLIER "Organization: OpenJS Foundation"
            DOWNLOAD_LOCATION https://www.npmjs.com
            LICENSE "Artistic-2.0"
            EXTREF "cpe:2.3:a:npmjs:npm:${NPM_VERSION}:*:*:*:*:*:*:*"
            RELATIONSHIP "@SBOM_LAST_SPDXID@ BUILD_TOOL_OF ${SBOM_LAST_SPDXID}"
        )
    endif()
endif()

if(WIN32)
    find_program(WIX_EXECUTABLE NAMES wix)
    if(WIX_EXECUTABLE)
        execute_process(
            COMMAND "${WIX_EXECUTABLE}" --version
            OUTPUT_VARIABLE WIX_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        string(REGEX REPLACE "\\+.*$" "" WIX_VERSION "${WIX_VERSION}")
    endif()
    if(WIX_VERSION)
        sbom_add(PACKAGE WiX
            VERSION "${WIX_VERSION}"
            SUPPLIER "Organization: WiX Toolset Contributors"
            DOWNLOAD_LOCATION https://wixtoolset.org
            LICENSE "MS-RL"
            EXTREF "cpe:2.3:a:wixtoolset:wix_toolset:${WIX_VERSION}:*:*:*:*:*:*:*"
        )
    endif()
endif()

sbom_add(PACKAGE libelectronic-id
    VERSION "${ELECTRONIC_ID_VERSION}"
    SUPPLIER "Organization: Estonian Information System Authority"
    DOWNLOAD_LOCATION https://github.com/web-eid/libelectronic-id
    LICENSE "MIT"
    EXTREF "cpe:2.3:a:web-eid:libelectronic-id:${ELECTRONIC_ID_VERSION}:*:*:*:*:*:*:*"
    RELATIONSHIP "${_app_spdxid}"
)

find_package(GTest QUIET)
if(GTest_FOUND)
    sbom_add(PACKAGE GTest
        VERSION "${GTest_VERSION}"
        SUPPLIER "Organization: Google LLC"
        DOWNLOAD_LOCATION https://github.com/google/googletest
        LICENSE "BSD-3-Clause"
        EXTREF "cpe:2.3:a:google:googletest:${GTest_VERSION}:*:*:*:*:*:*:*"
        RELATIONSHIP "${SBOM_LAST_SPDXID} TEST_TOOL_OF @SBOM_LAST_SPDXID@"
    )
endif()

if(PCSC_FOUND)
    sbom_add(PACKAGE libpcsclite
        VERSION "${PCSC_VERSION}"
        SUPPLIER "Organization: Muscle project"
        DOWNLOAD_LOCATION https://pcsclite.apdu.fr
        LICENSE "BSD-3-Clause"
        EXTREF "cpe:2.3:a:pcsc-lite_project:pcsc-lite:${PCSC_VERSION}:*:*:*:*:*:*:*"
    )
endif()

sbom_add(PACKAGE Qt6
    VERSION "${Qt6_VERSION}"
    SUPPLIER "Organization: The Qt Company"
    DOWNLOAD_LOCATION https://download.qt.io/
    LICENSE "LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only OR LicenseRef-Qt-commercial"
    EXTREF "cpe:2.3:a:qt:qt:${Qt6_VERSION}:*:*:*:*:*:*:*"
    RELATIONSHIP "${_app_spdxid}"
)

sbom_add(PACKAGE OpenSSL
    VERSION "${OPENSSL_VERSION}"
    SUPPLIER "Organization: OpenSSL Software Foundation"
    DOWNLOAD_LOCATION https://openssl.org
    LICENSE "Apache-2.0"
    EXTREF "cpe:2.3:a:openssl:openssl:${OPENSSL_VERSION}:*:*:*:*:*:*:*"
    RELATIONSHIP "${_app_spdxid}"
)

sbom_finalize(NO_VERIFY)
