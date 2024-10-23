add_library(ui STATIC
    certificatewidget.cpp
    certificatewidget.hpp
    punycode.hpp
    ui.cpp
    webeiddialog.cpp
    webeiddialog.hpp
    web-eid-resources.qrc
    dialog.ui
)
qt_add_translations(ui TS_FILES
    translations/en.ts
    translations/et.ts
    translations/fi.ts
    translations/hr.ts
    translations/ru.ts
    translations/de.ts
    translations/fr.ts
    translations/nl.ts
    translations/cs.ts
    translations/sk.ts
    RESOURCE_PREFIX /translations
)
set_target_properties(ui PROPERTIES
    AUTORCC ON
    AUTOUIC ON
)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC controller Qt6::SvgWidgets)
