if(${QT_VERSION} VERSION_LESS "5.15.0")
    macro(qt_add_translation)
        qt5_add_translation(${ARGN})
    endmacro()
    macro(qt_add_resources)
        qt5_add_resources(${ARGN})
    endmacro()
endif()

configure_file(translations/translations.qrc translations.qrc COPYONLY)
qt_add_translation(SOURCES
    translations/en.ts
    translations/et.ts
    translations/fi.ts
    translations/hr.ts
    translations/ru.ts
    translations/de.ts
    translations/fr.ts
    translations/nl.ts
)
add_library(ui STATIC
    ${SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    certificatewidget.cpp
    certificatewidget.hpp
    punycode.hpp
    ui.cpp
    webeiddialog.cpp
    webeiddialog.hpp
    web-eid-resources.qrc
    dialog.ui
)
set_target_properties(ui PROPERTIES
    AUTORCC ON
    AUTOUIC ON
)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui controller Qt${QT_VERSION_MAJOR}::Svg)
if(${QT_VERSION_MAJOR} STREQUAL "6")
    find_package(Qt6 COMPONENTS SvgWidgets REQUIRED)
    target_link_libraries(ui Qt6::SvgWidgets)
endif()
