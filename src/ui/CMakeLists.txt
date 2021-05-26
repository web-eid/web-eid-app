configure_file(translations/translations.qrc translations.qrc COPYONLY)
qt5_add_translation(SOURCES
    translations/en.ts
    translations/et.ts
    translations/fi.ts
    translations/ru.ts
)
add_library(ui STATIC
    ${SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc
    certificatelistwidget.cpp
    certificatelistwidget.hpp
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
target_link_libraries(ui controller Qt5::Svg)
