add_library(ui STATIC
    certificatewidget.cpp
    certificatewidget.hpp
    languageselect.cpp
    languageselect.hpp
    languageselect.ui
    punycode.hpp
    ui.cpp
    webeiddialog.cpp
    webeiddialog.hpp
    web-eid-resources.qrc
    dialog.ui
)
qt_add_resources(ui qss PREFIX / FILES dark.qss languageselect.qss)
file(GLOB TRANSLATIONS translations/*.ts)
qt_add_translations(ui TS_FILES ${TRANSLATIONS} RESOURCE_PREFIX /translations)
set_target_properties(ui PROPERTIES
    AUTORCC ON
    AUTOUIC ON
)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC controller Qt6::SvgWidgets)
