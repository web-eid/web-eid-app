// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "application.hpp"
#include "controller.hpp"
#include "logging.hpp"

#include <QTimer>

#include <iostream>

int main(int argc, char* argv[])
{
    Application app(argc, argv, QStringLiteral("web-eid"));

    try {
        Controller controller(app.parseArgs());

        QObject::connect(&controller, &Controller::quit, &app, &QApplication::quit);
        // Pass control to Controller::run() when the event loop starts.
        QTimer::singleShot(0, &controller, &Controller::run);

        return QApplication::exec();

    } catch (const ArgumentError& error) {
        // This error must go directly to cerr to avoid extra info from the logging system.
        std::cerr << error.what() << std::endl;
    } catch (const std::exception& error) {
        qCritical() << error;
    }

    return -1;
}
