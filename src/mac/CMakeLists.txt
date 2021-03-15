add_library(SafariServices INTERFACE)
target_compile_options(SafariServices INTERFACE -fobjc-arc)
target_link_options(SafariServices INTERFACE -fobjc-arc -fobjc-link-runtime)
target_link_libraries(SafariServices INTERFACE "-framework SafariServices")

add_executable(web-eid-safari MACOSX_BUNDLE main.mm ../app/getCommandHandler.cpp web-eid-safari.entitlements)
set_target_properties(web-eid-safari PROPERTIES
    AUTOMOC OFF
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info-app.plist.cmake
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/web-eid-safari.entitlements"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "ee.ria.web-eid-safari"
)
set_property(SOURCE main.mm APPEND PROPERTY COMPILE_DEFINITIONS PROJECT_VERSION="${SEMVER_VERSION_STRING}")
target_link_libraries(web-eid-safari SafariServices controller ui pcsc "-framework Cocoa")

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

add_executable(web-eid-safari-extension MACOSX_BUNDLE safari-extension.mm web-eid-safari-extension.entitlements ${EXTENSION})
set_target_properties(web-eid-safari-extension PROPERTIES
    AUTOMOC OFF
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info-extension.plist.cmake
    BUNDLE_EXTENSION appex
    RESOURCE "${EXTENSION}"
    XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/web-eid-safari-extension.entitlements"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "ee.ria.web-eid-safari.web-eid-safari-extension"
)
target_link_libraries(web-eid-safari-extension SafariServices "-framework AppKit")
target_link_options(web-eid-safari-extension PRIVATE -e _NSExtensionMain -fapplication-extension)

add_custom_command(TARGET web-eid-safari POST_BUILD
    COMMAND mkdir -p $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari>/PlugIns
    COMMAND cp -a $<TARGET_BUNDLE_DIR:web-eid-safari-extension> $<TARGET_BUNDLE_CONTENT_DIR:web-eid-safari>/PlugIns
)

install(TARGETS web-eid-safari DESTINATION /Applications/Utilities)
add_custom_target(safari-installer DEPENDS web-eid-safari
    COMMAND ${_qt5Core_install_prefix}/bin/macdeployqt $<TARGET_BUNDLE_DIR:web-eid-safari>
    COMMAND make install DESTDIR=root
    COMMAND pkgbuild --root root --identifier "eu.web-eid.$<TARGET_NAME:web-eid-safari>"
        --version "${PROJECT_VERSION}" $<TARGET_NAME:web-eid-safari>-${PROJECT_VERSION}.pkg
)
