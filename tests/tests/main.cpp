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

// https://bugreports.qt.io/browse/QTBUG-72073
#define QT_NO_FLOAT16_OPERATORS
#include "controller.hpp"
#include "registermetatypes.hpp"
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
    void getCertificate_expiredCertificateHasExpiredStatus();
    void getCertificate_outputsSupportedAlgos();

    void authenticate_validArgumentsResultInValidJwt();

    void fromPunycode_decodesEeDomain();

private:
    void runEventLoopVerifySignalsEmitted(QSignalSpy& actionSpy);
    void initGetCert();
    void initAuthenticate();
    void initCard();

    std::unique_ptr<Controller> controller;
};

void WebEidTests::initTestCase()
{
    registerMetatypes();
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
    runEventLoopVerifySignalsEmitted(statusUpdateSpy);

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
    QCOMPARE(certInfo.subject, QStringLiteral("M\u00C4NNIK,MARI-LIIS,61709210125"));

    const auto certBytes =
        QByteArray::fromBase64(controller->result()["certificate"].toString().toUtf8());
    const auto cert = QSslCertificate(certBytes, QSsl::EncodingFormat::Der);
    QVERIFY(!cert.isNull());
    QCOMPARE(cert.subjectInfo(QSslCertificate::CommonName)[0],
             QStringLiteral("M\u00C4NNIK,MARI-LIIS,61709210125"));
}

void WebEidTests::getCertificate_expiredCertificateHasExpiredStatus()
{
    // arrange
    PcscMock::setAtr(ESTEID_GEMALTO_V3_5_8_COLD_ATR);
    PcscMock::setApduScript(
        replaceCertValidUntilTo2010(ESTEID_GEMALTO_V3_5_8_GET_AUTH_CERTIFICATE_AND_AUTHENTICATE));

    initGetCert();

    QSignalSpy certificateFailureSpy(g_cached_GetCertificate.get(), &GetCertificate::retry);

    // act
    runEventLoopVerifySignalsEmitted(certificateFailureSpy);

    // assert
    const auto failure = qvariant_cast<RetriableError>(certificateFailureSpy.first().at(0));
    QCOMPARE(failure, RetriableError::NO_VALID_CERTIFICATE_AVAILABLE);
}

void WebEidTests::getCertificate_outputsSupportedAlgos()
{
    // arrange
    initCard();
    initGetCert();

    QSignalSpy certificateReadySpy(g_cached_GetCertificate.get(),
                                   &GetCertificate::singleCertificateReady);
    QVariantMap ES384_ALGO {
        {"crypto-algo", "ECC"}, {"hash-algo", "SHA-384"}, {"padding-algo", "NONE"}};

    // act
    runEventLoopVerifySignalsEmitted(certificateReadySpy);

    // assert
    QCOMPARE(controller->result()["supported-signature-algos"].toList()[0].toMap(), ES384_ALGO);
}

void WebEidTests::authenticate_validArgumentsResultInValidJwt()
{
    // arrange
    initCard();
    initAuthenticate();

    QSignalSpy authenticateSpy(g_cached_Authenticate.get(),
                               &GetCertificate::singleCertificateReady);

    // act
    runEventLoopVerifySignalsEmitted(authenticateSpy);

    // assert
    const auto certInfo = getCertAndPinInfoFromSignalSpy(authenticateSpy);
    QCOMPARE(certInfo.subject, QStringLiteral("M\u00C4NNIK,MARI-LIIS,61709210125"));

    QCOMPARE(
        QString(controller->result()["auth-token"].toString().toUtf8()).left(316),
        QStringLiteral(
            "eyJhbGciOiJFUzM4NCIsInR5cCI6IkpXVCIsIng1YyI6WyJNSUlHUnpDQ0JDK2dBd0lCQWdJUVJBN1gwUHlSRj"
            "I5WjFNMTJEZ3QreHpBTkJna3Foa2lHOXcwQkFRc0ZBREJyTVFzd0NRWURWUVFHRXdKRlJURWlNQ0FHQTFVRUNn"
            "d1pRVk1nVTJWeWRHbG1hWFJ6WldWeWFXMXBjMnRsYzJ0MWN6RVhNQlVHQTFVRVlRd09UbFJTUlVVdE1UQTNORG"
            "N3TVRNeEh6QWRCZ05WQkFNTUZsUkZVMVFnYjJZZ1JWTlVSVWxFTFZOTElE"));
}

void WebEidTests::fromPunycode_decodesEeDomain()
{
    QCOMPARE(fromPunycode(QUrl("https://xn--igusnunik-p7af.ee")),
             QStringLiteral("https://\u00F5igusn\u00F5unik.ee"));
}

void WebEidTests::runEventLoopVerifySignalsEmitted(QSignalSpy& actionSpy)
{
    // Waits until Controller emits quit.
    QSignalSpy quitSpy(controller.get(), &Controller::quit);

    // Pass control to Controller::run() when the event loop starts.
    QTimer::singleShot(0, controller.get(), &Controller::run);

    // Run the event loop, verify that signals were emitted.
    QVERIFY(actionSpy.wait());
    if (quitSpy.count() != 1) {
        QVERIFY(quitSpy.wait());
    }
}

void WebEidTests::initGetCert()
{
    try {
        auto getCertCmd = std::make_unique<CommandWithArguments>(CommandType::GET_CERTIFICATE,
                                                                 GET_CERTIFICATE_COMMAND_ARGUMENT);
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

void WebEidTests::initCard()
{
    try {
        PcscMock::setAtr(ESTEID_GEMALTO_V3_5_8_COLD_ATR);
        const auto notExpiredCertScript = replaceCertValidUntilToNextYear(
            ESTEID_GEMALTO_V3_5_8_GET_AUTH_CERTIFICATE_AND_AUTHENTICATE);
        PcscMock::setApduScript(notExpiredCertScript);

    } catch (const std::exception& e) {
        QFAIL(QStringLiteral("WebEidTests::initCard() failed with exception: %s")
                  .arg(e.what())
                  .toUtf8());
    }
}

QTEST_MAIN(WebEidTests)

#include "main.moc"
