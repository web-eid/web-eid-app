add_executable(web-eid WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/install/appicon.icns
    getcommandhandler.cpp
    InfoPlist.strings
    main.cpp
    web-eid.rc
    web-eid.entitlements
)
set_property(SOURCE web-eid.rc APPEND PROPERTY COMPILE_DEFINITIONS
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
)
target_link_libraries(web-eid controller ui pcsc)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM x64)
    else()
        set(PLATFORM x86)
    endif()
    find_package(OpenSSL REQUIRED)
    get_filename_component(SSL_PATH "${OPENSSL_CRYPTO_LIBRARY}" DIRECTORY)
    get_filename_component(SSL_PATH "${SSL_PATH}/../bin" ABSOLUTE)
    set(WEBEID_PATH web-eid.exe)
    set(BASE_FILE $<TARGET_NAME:web-eid>_${PROJECT_VERSION})
    set(MSI_FILE ${BASE_FILE}.${PLATFORM})
    list(APPEND CANDLE_CMD "$ENV{WIX}bin\\candle.exe" -nologo -arch ${PLATFORM}
        -dMSI_VERSION=${PROJECT_VERSION}
        -dPlatform=${PLATFORM}
        -dssl_path="${SSL_PATH}"
        -dqt_suffix="$<$<CONFIG:Debug>:d>"
        -djson=${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json
        -djsonfirefox=${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
        -dFIREFOX_URL="${FIREFOX_URL}"
        -dFIREFOX_UUID="${FIREFOX_UUID}"
        -dapp_path=$<TARGET_FILE:web-eid>
        ${CMAKE_SOURCE_DIR}/install/web-eid.wxs
        ${CMAKE_SOURCE_DIR}/install/WelcomeDlg2.wxs
        ${CMAKE_SOURCE_DIR}/install/WixUI_Minimal2.wxs
    )
    list(APPEND LIGHT_CMD "$ENV{WIX}bin\\light.exe" -nologo -ext WixUIExtension
        web-eid.wixobj WelcomeDlg2.wixobj WixUI_Minimal2.wixobj
        -dWixUIDialogBmp=${CMAKE_SOURCE_DIR}/install/dlgbmp.bmp
    )
    add_custom_target(installer DEPENDS web-eid
        COMMAND ${CANDLE_CMD}
        COMMAND ${LIGHT_CMD} -o "${MSI_FILE}.msi"
        #Build MSI with QT
        COMMAND ${CANDLE_CMD} -dqt_path=${_qt5Core_install_prefix}
        COMMAND ${LIGHT_CMD} -o "${MSI_FILE}.qt.msi"
        WORKING_DIRECTORY $<TARGET_FILE_DIR:web-eid>
    )
    add_custom_target(bundle
        COMMAND "$ENV{WIX}bin\\candle.exe" -nologo -ext WixBalExtension -ext WixUtilExtension -dwebeid="${BASE_FILE}"
            -dMSI_VERSION=${PROJECT_VERSION} -dpath="${CMAKE_SOURCE_DIR}/install" "${CMAKE_SOURCE_DIR}/install/plugins.wxs"
        COMMAND "$ENV{WIX}bin\\light.exe" "plugins.wixobj" -nologo -ext WixBalExtension -out "${BASE_FILE}.exe"
        WORKING_DIRECTORY $<TARGET_FILE_DIR:web-eid>
    )
    if(SIGNCERT)
        list(APPEND SIGNCMD signtool.exe sign /a /v /s MY /n "${SIGNCERT}" /fd SHA256
            /du http://installer.id.ee /tr http://sha256timestamp.ws.symantec.com/sha256/timestamp /td SHA256)
        add_custom_command(TARGET web-eid POST_BUILD
            COMMAND ${SIGNCMD} "$<$<BOOL:${CROSSSIGNCERT}>:/ph;/ac;${CROSSSIGNCERT}>" $<TARGET_FILE:web-eid>
            COMMAND_EXPAND_LISTS
        )
        add_custom_command(TARGET installer POST_BUILD
            COMMAND ${SIGNCMD} "${MSI_FILE}.msi" "${MSI_FILE}.qt.msi"
            WORKING_DIRECTORY $<TARGET_FILE_DIR:web-eid>
        )
        add_custom_command(TARGET bundle POST_BUILD
            COMMAND "$ENV{WIX}bin\\insignia.exe" -nologo -ib "${BASE_FILE}.exe" -o "${BASE_FILE}.engine.exe"
            COMMAND ${SIGNCMD} "${BASE_FILE}.engine.exe"
            COMMAND "$ENV{WIX}bin\\insignia.exe" -nologo -ab "${BASE_FILE}.engine.exe" "${BASE_FILE}.exe" -o "${BASE_FILE}.exe"
            COMMAND ${SIGNCMD} "${BASE_FILE}.exe"
            COMMAND del "${BASE_FILE}.engine.exe"
            WORKING_DIRECTORY $<TARGET_FILE_DIR:web-eid>
        )
    endif()
elseif(APPLE)
    set(WEBEID_PATH /Applications/Utilities/web-eid.app/Contents/MacOS/web-eid)
    set_source_files_properties(${CMAKE_SOURCE_DIR}/install/appicon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION Resources/en.lproj)
    set_target_properties(web-eid PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in)
    configure_file(${CMAKE_SOURCE_DIR}/install/macos-postinstall.in scripts/postinstall @ONLY)
    install(TARGETS web-eid DESTINATION /Applications/Utilities COMPONENT main)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json COMPONENT chrome
        DESTINATION "/Library/Google/Chrome/NativeMessagingHosts")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json COMPONENT firefox
        DESTINATION "/Library/Application Support/Mozilla/NativeMessagingHosts" RENAME eu.webeid.json)
    install(FILES ${CMAKE_SOURCE_DIR}/install/ncibgoaomkmdpilpocfeponihegamlic.json COMPONENT chrome
        DESTINATION "/Library/Application Support/Google/Chrome/External Extensions")
    add_custom_target(macdeployqt DEPENDS web-eid
        COMMAND ${_qt5Core_install_prefix}/bin/macdeployqt $<TARGET_BUNDLE_DIR:web-eid>
    )
    add_custom_target(installer-firefox DEPENDS macdeployqt
        COMMAND rm -rf root_firefox
        COMMAND DESTDIR=root_firefox ${CMAKE_COMMAND} --install . --component main
        COMMAND DESTDIR=root_firefox ${CMAKE_COMMAND} --install . --component firefox
        COMMAND pkgbuild --root root_firefox --scripts scripts --identifier "eu.web-eid.$<TARGET_NAME:web-eid>-firefox"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid>-firefox_${PROJECT_VERSION}.pkg
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(installer-chrome DEPENDS macdeployqt
        COMMAND rm -rf root_chrome
        COMMAND DESTDIR=root_chrome ${CMAKE_COMMAND} --install . --component main
        COMMAND DESTDIR=root_chrome ${CMAKE_COMMAND} --install . --component chrome
        COMMAND pkgbuild --root root_chrome --identifier "eu.web-eid.$<TARGET_NAME:web-eid>-chrome"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid>-chrome_${PROJECT_VERSION}.pkg
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(installer DEPENDS installer-chrome installer-firefox
        COMMAND mkdir -p root_bundle packages
        COMMAND cp web-eid-chrome_*.pkg packages/web-eid-chrome.pkg
        COMMAND cp web-eid-firefox_*.pkg packages/web-eid-firefox.pkg
        COMMAND pkgbuild --root root_bundle --identifier "eu.web-eid.web-eid"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" packages/web-eid-base.pkg
        COMMAND productbuild --distribution ${CMAKE_SOURCE_DIR}/install/distribution.xml
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>" --resources ${CMAKE_SOURCE_DIR}/install/
            --version "${PROJECT_VERSION}" --package-path packages/ web-eid_${PROJECT_VERSION}.pkg
        COMMAND mkdir -p tmp
        COMMAND cp ${CMAKE_SOURCE_DIR}/install/DS_Store tmp/.DS_Store
        COMMAND cp web-eid_${PROJECT_VERSION}.pkg tmp/web-eid.pkg
        COMMAND cp ${CMAKE_SOURCE_DIR}/install/background.png ${CMAKE_SOURCE_DIR}/install/uninstall.sh tmp
        COMMAND setfile -a E tmp/web-eid.pkg
        COMMAND setfile -a V tmp/background.png
        COMMAND hdiutil create web-eid_${PROJECT_VERSION}.dmg -ov -volname web-eid -srcfolder tmp
        COMMAND_EXPAND_LISTS
    )
    if(SIGNCERT)
        add_custom_command(TARGET macdeployqt POST_BUILD
            COMMAND codesign -f -o runtime --timestamp -s "Developer ID Application: ${SIGNCERT}"
                $<TARGET_BUNDLE_CONTENT_DIR:web-eid>/Frameworks/*.*
                $<TARGET_BUNDLE_CONTENT_DIR:web-eid>/PlugIns/*/*
            COMMAND codesign -f -o runtime --timestamp -s "Developer ID Application: ${SIGNCERT}"
                --entitlements ${CMAKE_CURRENT_LIST_DIR}/web-eid.entitlements
                $<TARGET_BUNDLE_DIR:web-eid>
        )
        add_custom_command(TARGET installer POST_BUILD
            COMMAND codesign -f --timestamp -s "Developer ID Application: ${SIGNCERT}" web-eid_${PROJECT_VERSION}.dmg
        )
    endif()
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
        DESTINATION "/usr/share/chrome-token-signing")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
        DESTINATION "${MOZILLA_INSTALL_DIR}" RENAME eu.webeid.json)
    install(FILES ${CMAKE_SOURCE_DIR}/install/ncibgoaomkmdpilpocfeponihegamlic.json
        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/google-chrome/extensions")
    install(FILES ${CMAKE_SOURCE_DIR}/install/web-eid.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    foreach(RES 16 32 128 256 512)
        install(
            FILES ${CMAKE_SOURCE_DIR}/install/appicon_${RES}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${RES}x${RES}/apps/
            RENAME web-eid.png
        )
    endforeach()
    add_custom_target(installer
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND dch --distribution unstable -v ${PROJECT_VERSION} "Release: ${PROJECT_VERSION}."
        COMMAND dpkg-buildpackage -us -uc
    )
endif()
configure_file(${CMAKE_SOURCE_DIR}/install/eu.webeid.json.cmake eu.webeid.json @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/install/eu.webeid.firefox.json.cmake eu.webeid.firefox.json @ONLY)
