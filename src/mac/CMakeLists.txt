# Helper
add_library(SafariServices INTERFACE)
target_compile_options(SafariServices INTERFACE -fobjc-arc)
target_link_options(SafariServices INTERFACE -fobjc-arc -fobjc-link-runtime)
target_link_libraries(SafariServices INTERFACE "-framework SafariServices")

# Extension JS
find_program(NPM_EXECUTABLE NAMES npm REQUIRED)
get_filename_component(BINPATH ${NPM_EXECUTABLE} PATH)
set(JSPATH ${CMAKE_CURRENT_SOURCE_DIR}/js/dist/safari)
set(EXTENSION ${JSPATH}/manifest.json ${JSPATH}/background.js ${JSPATH}/content.js ${JSPATH}/background.js.map ${JSPATH}/content.js.map ${JSPATH}/web-eid-poc-icon-128.png)
add_custom_command(OUTPUT ${EXTENSION}
    COMMAND ${CMAKE_COMMAND} -E env PATH="$ENV{PATH}:${BINPATH}" ${NPM_EXECUTABLE} install
    COMMAND ${CMAKE_COMMAND} -E env PATH="$ENV{PATH}:${BINPATH}" ${NPM_EXECUTABLE} run clean build
    COMMENT "Generating extension js"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/js
)

# Extension
add_executable(web-eid-safari-extension MACOSX_BUNDLE safari-extension.mm web-eid-safari-extension.entitlements ${EXTENSION})
set_target_properties(web-eid-safari-extension PROPERTIES
    AUTOMOC OFF
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info-extension.plist.cmake
    BUNDLE_EXTENSION appex
    XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/web-eid-safari-extension.entitlements"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "ee.ria.web-eid-safari.web-eid-safari-extension"
)
target_link_libraries(web-eid-safari-extension SafariServices "-framework AppKit")
target_link_options(web-eid-safari-extension PRIVATE -e _NSExtensionMain -fapplication-extension)
add_custom_command(TARGET web-eid-safari-extension POST_BUILD
    COMMAND mkdir -p $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari-extension>/Resources
    COMMAND cp ${EXTENSION} $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari-extension>/Resources
)

# Application
add_executable(web-eid-safari MACOSX_BUNDLE main.mm ../app/getCommandHandler.cpp web-eid-safari.entitlements)
set_target_properties(web-eid-safari PROPERTIES
    AUTOMOC OFF
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info-app.plist.cmake
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/web-eid-safari.entitlements"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "ee.ria.web-eid-safari"
)
target_link_libraries(web-eid-safari SafariServices controller ui pcsc "-framework Cocoa")
add_custom_command(TARGET web-eid-safari POST_BUILD
    COMMAND mkdir -p $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari>/PlugIns
    COMMAND cp -a $<TARGET_BUNDLE_DIR:web-eid-safari-extension> $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari>/PlugIns
)
install(TARGETS web-eid-safari DESTINATION /Applications/Utilities)
add_custom_target(macdeployqt-safari DEPENDS web-eid-safari
    COMMAND ${_qt5Core_install_prefix}/bin/macdeployqt $<TARGET_BUNDLE_DIR:web-eid-safari>
)

# Installer
add_custom_target(installer-safari DEPENDS macdeployqt-safari
    COMMAND make install DESTDIR=root_safari
    COMMAND pkgbuild --root root_safari --identifier "eu.web-eid.$<TARGET_NAME:web-eid-safari>"
        "$<$<BOOL:${SIGNCERT}>:--sign;${SIGNCERT}>"
        --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid-safari>_${PROJECT_VERSION}.pkg
    COMMAND_EXPAND_LISTS
)
add_custom_target(installer DEPENDS installer-chrome installer-firefox installer-safari
    COMMAND mkdir -p root_bundle packages
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/../app/web-eid-chrome_*.pkg packages/web-eid-chrome.pkg
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/../app/web-eid-firefox_*.pkg packages/web-eid-firefox.pkg
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/web-eid-safari_*.pkg packages/web-eid-safari.pkg
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

# Sign
if(SIGNCERT)
    add_custom_command(TARGET web-eid-safari-extension POST_BUILD
        COMMAND codesign -f -o runtime --timestamp -s "Developer ID Application: ${SIGNCERT}"
            --entitlements ${CMAKE_CURRENT_LIST_DIR}/web-eid-safari-extension.entitlements
            $<TARGET_BUNDLE_DIR:web-eid-safari-extension>
    )
    add_custom_command(TARGET macdeployqt-safari POST_BUILD
        COMMAND codesign -f -o runtime --timestamp -s "Developer ID Application: ${SIGNCERT}"
            $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari>/Frameworks/*.*
            $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari>/PlugIns/*/*.dylib
        COMMAND codesign -f -o runtime --timestamp -s "Developer ID Application: ${SIGNCERT}"
            --entitlements ${CMAKE_CURRENT_LIST_DIR}/web-eid-safari.entitlements
            $<TARGET_BUNDLE_DIR:web-eid-safari>
    )
    add_custom_command(TARGET installer POST_BUILD
        COMMAND codesign -f --timestamp -s "Developer ID Application: ${SIGNCERT}" web-eid_${PROJECT_VERSION}.dmg
    )
endif()
