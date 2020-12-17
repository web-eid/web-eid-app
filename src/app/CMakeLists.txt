set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION_TWEAK})

set(SEMVER_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}+${PROJECT_VERSION_TWEAK}")

add_executable(web-eid WIN32 MACOSX_BUNDLE
    getcommandhandler.cpp
    main.cpp
    parseargs.hpp
    web-eid.rc
)
set_property(SOURCE web-eid.rc APPEND PROPERTY COMPILE_DEFINITIONS
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
)
set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_DEFINITIONS PROJECT_VERSION="${SEMVER_VERSION_STRING}")
target_link_libraries(web-eid controller ui pcsc)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM x64)
        set(SSL_PATH "C:\\OpenSSL-Win64\\bin")
    else()
        set(PLATFORM x86)
        set(SSL_PATH "C:\\OpenSSL-Win32\\bin")
    endif()
    if(OPENSSL_ROOT_DIR)
        set(SSL_PATH "${OPENSSL_ROOT_DIR}\\bin")
    endif()
    set(WEBEID_PATH web-eid.exe)
    add_custom_target(installer DEPENDS web-eid
        COMMAND "$ENV{WIX}bin\\candle.exe" -nologo -arch ${PLATFORM} -dMSI_VERSION=${PROJECT_VERSION} -dPlatform=${PLATFORM}
            -dssl_path="${SSL_PATH}" -dqt_suffix="$<$<CONFIG:Debug>:d>" -dqt_path=${_qt5Core_install_prefix}
            -djson=${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json
            -djsonfirefox=${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
            -dFIREFOX_URL="${FIREFOX_URL}" -dFIREFOX_UUID="${FIREFOX_UUID}"
            -dapp_path=$<TARGET_FILE:web-eid> ${CMAKE_SOURCE_DIR}/install/web-eid.wxs
        COMMAND "$ENV{WIX}bin\\light.exe" -nologo -ext WixUIExtension web-eid.wixobj -o $<TARGET_FILE_NAME:web-eid>-${PROJECT_VERSION}.${PLATFORM}.msi
        WORKING_DIRECTORY $<TARGET_FILE_DIR:web-eid>
    )
elseif(APPLE)
    set(WEBEID_PATH /Applications/Utilities/web-eid.app/Contents/MacOS/web-eid)
    configure_file(MacOSXBundleInfo.plist.in MacOSXBundleInfo.plist)
    set_target_properties(web-eid PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/MacOSXBundleInfo.plist)
    configure_file(${CMAKE_SOURCE_DIR}/install/macos-postinstall.in scripts/postinstall @ONLY)
    install(TARGETS web-eid DESTINATION /Applications/Utilities)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json
        DESTINATION "/Library/Google/Chrome/NativeMessagingHosts")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
        DESTINATION "/Library/Application Support/Mozilla/NativeMessagingHosts" RENAME eu.webeid.json)
    install(FILES ${CMAKE_SOURCE_DIR}/install/ncibgoaomkmdpilpocfeponihegamlic.json
        DESTINATION "/Library/Application Support/Google/Chrome/External Extensions")
    add_custom_target(installer DEPENDS web-eid
        COMMAND ${_qt5Core_install_prefix}/bin/macdeployqt ${CMAKE_CURRENT_BINARY_DIR}/web-eid.app
        COMMAND make install DESTDIR=root
        COMMAND pkgbuild --root root --scripts scripts --identifier "eu.web-eid.$<TARGET_FILE_NAME:web-eid>"
            --version "${PROJECT_VERSION}" $<TARGET_FILE_NAME:web-eid>-${PROJECT_VERSION}.pkg
    )
else()
    include(GNUInstallDirs)
    set(WEBEID_PATH ${CMAKE_INSTALL_FULL_BINDIR}/web-eid)
    install(TARGETS web-eid DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(MOZILLA_INSTALL_DIR)
        message("Using defined mozilla dir: ${MOZILLA_INSTALL_DIR}")
    elseif(NOT EXISTS "/etc/debian_version" AND "${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(MOZILLA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib64/mozilla/native-messaging-hosts")
    else()
        set(MOZILLA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/mozilla/native-messaging-hosts")
    endif()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json
        DESTINATION "/etc/opt/chrome/native-messaging-hosts")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
        DESTINATION "${MOZILLA_INSTALL_DIR}" RENAME eu.webeid.json)
    install(FILES ${CMAKE_SOURCE_DIR}/install/ncibgoaomkmdpilpocfeponihegamlic.json
        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/google-chrome/extensions")
    add_custom_target(installer
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND dh_make --createorig --addmissing --defaultless -y -p $<TARGET_FILE_NAME:web-eid>_${PROJECT_VERSION}
        COMMAND dch --distribution unstable -v ${PROJECT_VERSION} "Release: ${PROJECT_VERSION}."
        COMMAND dpkg-buildpackage -rfakeroot -us -uc
    )
endif()
configure_file(${CMAKE_SOURCE_DIR}/install/eu.webeid.json.cmake eu.webeid.json @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/install/eu.webeid.firefox.json.cmake eu.webeid.firefox.json @ONLY)
