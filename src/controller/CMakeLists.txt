add_library(controller STATIC
    application.cpp
    application.hpp
    certandpininfo.hpp
    command-handlers/authenticate.cpp
    command-handlers/authenticate.hpp
    command-handlers/certificatereader.cpp
    command-handlers/certificatereader.hpp
    command-handlers/getcertificate.cpp
    command-handlers/getcertificate.hpp
    command-handlers/sign.cpp
    command-handlers/sign.hpp
    command-handlers/signauthutils.cpp
    command-handlers/signauthutils.hpp
    commandhandler.hpp
    commands.cpp
    commands.hpp
    controller.cpp
    controller.hpp
    inputoutputmode.cpp
    inputoutputmode.hpp
    logging.cpp
    logging.hpp
    qeid.hpp
    retriableerror.cpp
    retriableerror.hpp
    threads/cardeventmonitorthread.hpp
    threads/commandhandlerconfirmthread.hpp
    threads/commandhandlerrunthread.hpp
    threads/controllerchildthread.cpp
    threads/controllerchildthread.hpp
    threads/waitforcardthread.hpp
    ui.hpp
    utils/erasedata.hpp
    utils/observer_ptr.hpp
    utils/qdisablecopymove.hpp
    utils/utils.hpp
    writeresponse.cpp
    writeresponse.hpp
)

if(APPLE)
    target_sources(controller PRIVATE application.mm)
    set_source_files_properties(application.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(controller "-framework Cocoa" "-framework AppKit" "-fobjc-arc")
endif()

set_property(SOURCE application.cpp APPEND PROPERTY COMPILE_DEFINITIONS PROJECT_VERSION="${SEMVER_VERSION_STRING}")
target_include_directories(controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(controller
    electronic-id
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
)

# %{function}:%{file}:%{line} works in Qt log message pattern only if code
# is compiled in debug mode or if QT_MESSAGELOGCONTEXT is set in compiler flags.
target_compile_definitions(controller PUBLIC QT_MESSAGELOGCONTEXT)
