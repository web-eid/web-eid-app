add_library(ui STATIC
    certificatewidget.cpp
    certificatewidget.hpp
    punycode.hpp
    ui.cpp
    webeiddialog.cpp
    webeiddialog.hpp
    dialog.ui
)
file(GLOB IMAGES images/*.svg)
file(GLOB FONTS fonts/*.ttf)
qt_add_resources(ui resources BASE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX / FILES
    ../../install/appicon_128.png
    dark.qss
    ${IMAGES}
    ${FONTS}
)
file(GLOB TRANSLATIONS translations/*.ts)
qt_add_translations(ui
    TS_FILES ${TRANSLATIONS}
    RESOURCE_PREFIX /translations
    LUPDATE_OPTIONS -locations none
)
set_target_properties(ui PROPERTIES
    AUTORCC ON
    AUTOUIC ON
)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC controller Qt6::SvgWidgets)
