add_library(controller STATIC
    certificateinfo.hpp
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
    logging.hpp
    logging.cpp
    qeid.hpp
    readermonitorthread.hpp
    registermetatypes.cpp
    registermetatypes.hpp
    ui.hpp
    utils.hpp
    writeresponse.cpp
    writeresponse.hpp
)

target_include_directories(controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(controller electronic-id Qt5::Network Qt5::Widgets)

# %{function}:%{file}:%{line} works in Qt log message pattern only if code
# is compiled in debug mode or if QT_MESSAGELOGCONTEXT is set in compiler flags.
target_compile_definitions(controller PUBLIC QT_MESSAGELOGCONTEXT)
