/*
 * Copyright (c) 2020-2023 Estonian Information System Authority
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

// https://bugreports.qt.io/browse/QTBUG-72073
#define QT_NO_FLOAT16_OPERATORS
#include "application.hpp"
#include "controller.hpp"
#include "command-handlers/getcertificate.hpp"

#include "../ui/punycode.hpp"

#include "mock-ui.hpp"
#include "getcommandhandler-mock.hpp"

#include "select-certificate-script.hpp"
#include "atrs.hpp"
#include "changecertificatevaliduntil.hpp"

QT_WARNING_PUSH
QT_WARNING_DISABLE_MSVC(4127)
#include <QtTest>
QT_WARNING_POP
#include <QTimer>
#include <QDebug>

namespace
{

CertificateInfo getCertAndPinInfoFromSignalSpy(const QSignalSpy& certificateReadySpy)
{
    const auto certInfosArgument =
        qvariant_cast<CardCertificateAndPinInfo>(certificateReadySpy.first().at(1));
    return certInfosArgument.certInfo;
}

} // namespace

class WebEidTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void statusUpdate_withUnsupportedCard_hasExpectedStatus();

    void getCertificate_validCertificateHasExpectedCertificateSubject();
    void getCertificate_expiredCertificateHasExpectedCertificateSubject();
    void getCertificate_outputsSupportedAlgos();

    void authenticate_validArgumentsResultInValidToken();

    void fromPunycode_decodesEeDomain();

    void quit_exits();

private:
    void runEventLoopVerifySignalsEmitted(QSignalSpy& actionSpy, bool waitForQuit = true);
    void initGetCert();
    void initAuthenticate();
    void initCard(bool withSigningScript = true);

    std::unique_ptr<Controller> controller;
};

void WebEidTests::initTestCase()
{
    Application::registerMetatypes();
}

void WebEidTests::init()
{
    PcscMock::reset();
}

void WebEidTests::statusUpdate_withUnsupportedCard_hasExpectedStatus()
{
    // arrange
    initGetCert();

    QSignalSpy statusUpdateSpy(controller.get(), &Controller::statusUpdate);

    // act
    runEventLoopVerifySignalsEmitted(statusUpdateSpy, false);

    // assert
    const auto statusArgument = qvariant_cast<RetriableError>(statusUpdateSpy.first().at(0));
    QCOMPARE(statusArgument, RetriableError::UNSUPPORTED_CARD);
}

void WebEidTests::getCertificate_validCertificateHasExpectedCertificateSubject()
{
    // arrange
    initCard();
    initGetCert();

    QSignalSpy certificateReadySpy(g_cached_GetCertificate.get(),
                                   &GetCertificate::singleCertificateReady);

    // act
    runEventLoopVerifySignalsEmitted(certificateReadySpy);

    // assert
    const auto certInfo = getCertAndPinInfoFromSignalSpy(certificateReadySpy);
    QCOMPARE(certInfo.subject, QStringLiteral("M\u00C4NNIK, MARI-LIIS, 61709210125"));

    const auto certBytes =
        QByteArray::fromBase64(controller->result()["certificate"].toString().toUtf8());
    const auto cert = QSslCertificate(certBytes, QSsl::EncodingFormat::Der);
    QVERIFY(!cert.isNull());
    QCOMPARE(cert.subjectInfo(QSslCertificate::CommonName)[0],
             QStringLiteral("M\u00C4NNIK,MARI-LIIS,61709210125"));
}

void WebEidTests::getCertificate_expiredCertificateHasExpectedCertificateSubject()
{
    // arrange
    PcscMock::setAtr(ESTEID_GEMALTO_V3_5_8_COLD_ATR);
    PcscMock::setApduScript(
        replaceCertValidUntilTo2010(ESTEID_GEMALTO_V3_5_8_GET_SIGN_CERTIFICATE_AND_SIGNING));

    initGetCert();

    QSignalSpy certificateReadySpy(g_cached_GetCertificate.get(),
                                   &GetCertificate::singleCertificateReady);

    // act
    runEventLoopVerifySignalsEmitted(certificateReadySpy);

    // assert
    const auto certInfo = getCertAndPinInfoFromSignalSpy(certificateReadySpy);
    QCOMPARE(certInfo.subject, QStringLiteral("M\u00C4NNIK, MARI-LIIS, 61709210125"));

    const auto certBytes =
        QByteArray::fromBase64(controller->result()["certificate"].toString().toUtf8());
    const auto cert = QSslCertificate(certBytes, QSsl::EncodingFormat::Der);
    QVERIFY(!cert.isNull());
    QCOMPARE(cert.subjectInfo(QSslCertificate::CommonName)[0],
             QStringLiteral("M\u00C4NNIK,MARI-LIIS,61709210125"));
}

void WebEidTests::getCertificate_outputsSupportedAlgos()
{
    // arrange
    initCard();
    initGetCert();

    QSignalSpy certificateReadySpy(g_cached_GetCertificate.get(),
                                   &GetCertificate::singleCertificateReady);
    QVariantMap ES224_ALGO {
        {"cryptoAlgorithm", "ECC"}, {"hashFunction", "SHA-224"}, {"paddingScheme", "NONE"}};

    // act
    runEventLoopVerifySignalsEmitted(certificateReadySpy);

    // assert
    QCOMPARE(controller->result()["supportedSignatureAlgorithms"].toList()[0].toMap(), ES224_ALGO);
}

void WebEidTests::authenticate_validArgumentsResultInValidToken()
{
    // arrange
    initCard(false);
    initAuthenticate();

    QSignalSpy authenticateSpy(g_cached_Authenticate.get(),
                               &GetCertificate::singleCertificateReady);

    // act
    runEventLoopVerifySignalsEmitted(authenticateSpy);

    // assert
    const auto certInfo = getCertAndPinInfoFromSignalSpy(authenticateSpy);
    QCOMPARE(certInfo.subject, QStringLiteral("M\u00C4NNIK, MARI-LIIS, 61709210125"));

    QCOMPARE(controller->result()["unverifiedCertificate"].toString().left(25),
             QStringLiteral("MIIGRzCCBC+gAwIBAgIQRA7X0"));
}

void WebEidTests::fromPunycode_decodesEeDomain()
{
    QCOMPARE(fromPunycode(QUrl("https://xn--igusnunik-p7af.ee")),
             QStringLiteral("\u00F5igusn\u00F5unik.ee"));
}

void WebEidTests::quit_exits()
{
    try {
        auto quitCmd = std::make_unique<CommandWithArguments>(CommandType::QUIT, QVariantMap {});
        controller = std::make_unique<Controller>(std::move(quitCmd));

        QSignalSpy quitSpy(controller.get(), &Controller::quit);
        QTimer::singleShot(0, controller.get(), &Controller::run);
        QVERIFY(quitSpy.wait());

    } catch (const std::exception& e) {
        QFAIL(QStringLiteral("WebEidTests::quit_exits() failed with exception: %s")
                  .arg(e.what())
                  .toUtf8());
    }
}

void WebEidTests::runEventLoopVerifySignalsEmitted(QSignalSpy& actionSpy, bool waitForQuit)
{
    // Waits until Controller emits quit.
    QSignalSpy quitSpy(controller.get(), &Controller::quit);

    // Pass control to Controller::run() when the event loop starts.
    QTimer::singleShot(0, controller.get(), &Controller::run);

    // Run the event loop, verify that signals were emitted.
    QVERIFY(actionSpy.wait());
    if (waitForQuit && quitSpy.count() < 1) {
        QVERIFY(quitSpy.wait());
    }
}

void WebEidTests::initGetCert()
{
    try {
        auto getCertCmd = std::make_unique<CommandWithArguments>(
            CommandType::GET_SIGNING_CERTIFICATE, GET_CERTIFICATE_COMMAND_ARGUMENT);
        // GetCertificate will make an internal copy of getCertCmd.
        g_cached_GetCertificate = std::make_unique<GetCertificate>(*getCertCmd);
        // Controller will take ownership of getCertCmd, it will be null after this line.
        controller = std::make_unique<Controller>(std::move(getCertCmd));

    } catch (const std::exception& e) {
        QFAIL(QStringLiteral("WebEidTests::initGetCert() failed with exception: %s")
                  .arg(e.what())
                  .toUtf8());
    }
}

void WebEidTests::initAuthenticate()
{
    try {
        auto authCmd = std::make_unique<CommandWithArguments>(CommandType::AUTHENTICATE,
                                                              AUTHENTICATE_COMMAND_ARGUMENT);
        // See comments in initGetCert() regarding authCmd lifetime.
        g_cached_Authenticate = std::make_unique<Authenticate>(*authCmd);
        controller = std::make_unique<Controller>(std::move(authCmd));

    } catch (const std::exception& e) {
        QFAIL(QStringLiteral("WebEidTests::initAuthenticate() failed with exception: %s")
                  .arg(e.what())
                  .toUtf8());
    }
}

void WebEidTests::initCard(bool withSigningScript)
{
    try {
        PcscMock::setAtr(ESTEID_GEMALTO_V3_5_8_COLD_ATR);
        const auto notExpiredCertScript = replaceCertValidUntilToNextYear(
            withSigningScript ? ESTEID_GEMALTO_V3_5_8_GET_SIGN_CERTIFICATE_AND_SIGNING
                              : ESTEID_GEMALTO_V3_5_8_GET_AUTH_CERTIFICATE_AND_AUTHENTICATE);
        PcscMock::setApduScript(notExpiredCertScript);

    } catch (const std::exception& e) {
        QFAIL(QStringLiteral("WebEidTests::initCard() failed with exception: %s")
                  .arg(e.what())
                  .toUtf8());
    }
}

QTEST_MAIN(WebEidTests)

#include "main.moc"
