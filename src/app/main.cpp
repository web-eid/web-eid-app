/*
 * Copyright (c) 2020 The Web eID Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "application.hpp"
#include "controller.hpp"
#include "logging.hpp"

#include <QTimer>

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(web_eid_resources);
    Q_INIT_RESOURCE(translations);

    Application app(argc, argv, QStringLiteral("web-eid"), QStringLiteral("Web eID"));

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
