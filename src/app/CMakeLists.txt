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
    if(OPENSSL_ROOT_DIR)
        set(SSL_PATH "${OPENSSL_ROOT_DIR}/bin")
    else()
        get_filename_component(SSL_PATH "${OPENSSL_INCLUDE_DIR}/../bin" ABSOLUTE)
    endif()
    set(WEBEID_PATH web-eid.exe)
    set(BASE_FILE $<TARGET_NAME:web-eid>_${PROJECT_VERSION}.${PLATFORM})
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
        COMMAND ${LIGHT_CMD} -o "${BASE_FILE}.msi"
        #Build MSI with QT
        COMMAND ${CANDLE_CMD} -dqt_path=${qtCore_install_prefix} -dqt_version_major=${QT_VERSION_MAJOR}
        COMMAND ${LIGHT_CMD} -o "${BASE_FILE}.qt.msi"
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
            COMMAND ${SIGNCMD} "${BASE_FILE}.msi" "${BASE_FILE}.qt.msi"
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
    configure_file(${CMAKE_SOURCE_DIR}/install/macos-postinstall.in root_firefox_scripts/postinstall @ONLY)
    install(TARGETS web-eid DESTINATION /Applications/Utilities COMPONENT main)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json COMPONENT chrome
        DESTINATION "/Library/Google/Chrome/NativeMessagingHosts")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json COMPONENT firefox
        DESTINATION "/Library/Application Support/Mozilla/NativeMessagingHosts" RENAME eu.webeid.json)
    install(FILES ${CMAKE_SOURCE_DIR}/install/ncibgoaomkmdpilpocfeponihegamlic.json COMPONENT chrome
        DESTINATION "/Library/Application Support/Google/Chrome/External Extensions")
    add_custom_target(macdeployqt DEPENDS web-eid
        COMMAND ${qtCore_install_prefix}/macdeployqt $<TARGET_BUNDLE_DIR:web-eid>
    )
    add_custom_target(installer-native DEPENDS macdeployqt
        COMMAND rm -rf root_native
        COMMAND DESTDIR=root_native ${CMAKE_COMMAND} --install . --component main
        COMMAND pkgbuild --root root_native --identifier "eu.web-eid.$<TARGET_NAME:web-eid>"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid>-native_${PROJECT_VERSION}.pkg
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(installer-firefox DEPENDS installer-native
        COMMAND rm -rf root_firefox
        COMMAND DESTDIR=root_firefox ${CMAKE_COMMAND} --install . --component firefox
        COMMAND pkgbuild --root root_firefox --scripts root_firefox_scripts --identifier "eu.web-eid.$<TARGET_NAME:web-eid>-firefox"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid>-firefox_${PROJECT_VERSION}.pkg
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(installer-chrome DEPENDS installer-native
        COMMAND rm -rf root_chrome
        COMMAND DESTDIR=root_chrome ${CMAKE_COMMAND} --install . --component chrome
        COMMAND pkgbuild --root root_chrome --identifier "eu.web-eid.$<TARGET_NAME:web-eid>-chrome"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid>-chrome_${PROJECT_VERSION}.pkg
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(installer-chrome-policy
        COMMAND rm -rf root_chrome_policy root_chrome_policy_scripts
        COMMAND mkdir -p root_chrome_policy root_chrome_policy_scripts
        COMMAND cp ${CMAKE_SOURCE_DIR}/install/macos-chrome-policy.plist root_chrome_policy_scripts/chrome-policy.plist
        COMMAND cp ${CMAKE_SOURCE_DIR}/install/macos-postinstall-chrome-policy root_chrome_policy_scripts/postinstall
        COMMAND /bin/sh -c "\"$<$<BOOL:${SIGNCERT}>:security;cms;-S;-N;Developer ID Application: ${SIGNCERT};-i;${CMAKE_SOURCE_DIR}/install/macos-chrome-policy.plist;-o;root_chrome_policy_scripts/chrome-policy.plist>\""
        COMMAND pkgbuild --root root_chrome_policy --scripts root_chrome_policy_scripts --identifier "eu.web-eid.$<TARGET_NAME:web-eid>-chrome-policy"
            "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
            --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid>-chrome-policy_${PROJECT_VERSION}.pkg
        COMMAND_EXPAND_LISTS
    )
    add_custom_target(installer DEPENDS installer-chrome installer-chrome-policy installer-firefox
        COMMAND mkdir -p packages
        COMMAND cp web-eid-native_*.pkg packages/web-eid-native.pkg
        COMMAND cp web-eid-chrome_*.pkg packages/web-eid-chrome.pkg
        COMMAND cp web-eid-chrome-policy_*.pkg packages/web-eid-chrome-policy.pkg
        COMMAND cp web-eid-firefox_*.pkg packages/web-eid-firefox.pkg
        COMMAND productbuild --distribution ${CMAKE_SOURCE_DIR}/install/distribution.xml
            "$<$<BOOL:${SIGNCERT}>:--sign;Developer ID Installer: ${SIGNCERT}>" --resources ${CMAKE_SOURCE_DIR}/install/
            --version "${PROJECT_VERSION}" --package-path packages/ web-eid_${PROJECT_VERSION}.pkg
        COMMAND mkdir -p tmp
        COMMAND cp ${CMAKE_SOURCE_DIR}/install/DS_Store tmp/.DS_Store
        COMMAND cp web-eid_${PROJECT_VERSION}.pkg tmp/Web-eID.pkg
        COMMAND cp ${CMAKE_SOURCE_DIR}/install/background.png ${CMAKE_SOURCE_DIR}/install/uninstall.sh tmp
        COMMAND setfile -a E tmp/Web-eID.pkg
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
    file(DOWNLOAD ${FIREFOX_URL} ${CMAKE_CURRENT_BINARY_DIR}/${FIREFOX_UUID}.xpi)
    set(WEBEID_PATH ${CMAKE_INSTALL_BINDIR}/web-eid)
    install(TARGETS web-eid DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(EXISTS "/etc/debian_version")
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/mozilla/native-messaging-hosts RENAME eu.webeid.json)
    else()
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.firefox.json
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/mozilla/native-messaging-hosts RENAME eu.webeid.json)
    endif()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eu.webeid.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/web-eid)
    install(FILES ${CMAKE_SOURCE_DIR}/install/ncibgoaomkmdpilpocfeponihegamlic.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/google-chrome/extensions)
    install(FILES ${CMAKE_SOURCE_DIR}/install/web-eid.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FIREFOX_UUID}.xpi
            DESTINATION ${CMAKE_INSTALL_DATADIR}/mozilla/extensions/{ec8030f7-c20a-464f-9b0e-13a3a9e97384})
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
