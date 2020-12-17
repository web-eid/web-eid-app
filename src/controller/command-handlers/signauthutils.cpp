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

#include "signauthutils.hpp"

#include "utils.hpp"

using namespace electronic_id;

template <typename T>
T validateAndGetArgument(const QString& argName, const QVariantMap& args, bool allowNull)
{
    // FIXME: implement argument validation with custom exceptions when adding stdin mode.
    if (!args.contains(argName)) {
        throw std::invalid_argument("validateAndGetArgument(): argument '" + argName.toStdString()
                                    + "' is missing");
    }
    if (allowNull && args[argName].isNull()) {
        return args[argName].value<T>();
    }
    const auto argValue = args[argName].value<T>();
    if (argValue.isEmpty()) {
        throw std::invalid_argument("validateAndGetArgument(): argument '" + argName.toStdString()
                                    + "' is empty");
    }
    return argValue;
}

template QString validateAndGetArgument<QString>(const QString& argName, const QVariantMap& args,
                                                 bool allowNull);
template QByteArray validateAndGetArgument<QByteArray>(const QString& argName,
                                                       const QVariantMap& args, bool allowNull);

QSslCertificate parseAndValidateCertificate(const QString& certArgName, const QVariantMap& args,
                                            bool allowNull)
{
    // FIXME: implement argument validation with custom exceptions when adding stdin mode.
    const auto certStr = validateAndGetArgument<QString>(certArgName, args, allowNull);
    if (allowNull && certStr.isNull()) {
        return QSslCertificate();
    }
    const auto cert =
        QSslCertificate(QByteArray::fromBase64(certStr.toUtf8()), QSsl::EncodingFormat::Der);
    if (cert.isNull()) {
        throw std::invalid_argument(
            "parseAndValidateCertificate: invalid certificate passed as argument");
    }

    return cert;
}

pcsc_cpp::byte_vector getPin(QObject* window, const QString& pinInputName)
{
    requireNonNull(window, "window", "getPin");

    // FIXME: keep PIN in secure memory, implement custom Qt widget.
    auto pinInput = window->findChild<QObject*>(pinInputName);
    if (!pinInput) {
        throw std::logic_error("getPin(): " + pinInputName.toStdString()
                               + " not found in window object");
    }
    auto pin = pinInput->property("text").toString();

    // FIXME: Overwrite PIN memory in widget properly, this is really just a placeholder.
    pinInput->setProperty("text", QStringLiteral("123456789012345678901234567890"));
    // Clear PIN input so that it does not show placeholders.
    pinInput->setProperty("text", QString());

    // FIXME: Avoid making copies of the PIN in memory.
    auto pinQByteArray = pin.toUtf8();
    auto pinBytes = pcsc_cpp::byte_vector {pinQByteArray.begin(), pinQByteArray.end()};

    pin.fill('\0');
    pinQByteArray.fill('\0');

    return pinBytes;
}

QVariantMap signatureAlgoToVariantMap(const SignatureAlgorithm signatureAlgo)
{
    const static QMap<HashAlgorithm, QVariant> HASHALGO_TO_VARIANT {
        {HashAlgorithm::SHA224, "SHA-224"},    {HashAlgorithm::SHA256, "SHA-256"},
        {HashAlgorithm::SHA384, "SHA-384"},    {HashAlgorithm::SHA512, "SHA-512"},
        {HashAlgorithm::SHA3_224, "SHA3-224"}, {HashAlgorithm::SHA3_256, "SHA3-256"},
        {HashAlgorithm::SHA3_384, "SHA3-384"}, {HashAlgorithm::SHA3_512, "SHA3-512"},
    };

    QVariant hashAlgo = HASHALGO_TO_VARIANT.value(signatureAlgo);
    if (hashAlgo.isNull()) {
        throw std::logic_error("Unknown hash algorithm for signature algorithm "
                               + std::string(signatureAlgo));
    }

    if (signatureAlgo & SignatureAlgorithm::ES)
        return {{"crypto-algo", "ECC"}, {"hash-algo", hashAlgo}, {"padding-algo", "NONE"}};
    if (signatureAlgo & SignatureAlgorithm::RS)
        return {{"crypto-algo", "RSA"}, {"hash-algo", hashAlgo}, {"padding-algo", "PKCS1.5"}};
    if (signatureAlgo & SignatureAlgorithm::PS)
        return {{"crypto-algo", "RSA"}, {"hash-algo", hashAlgo}, {"padding-algo", "PSS"}};
    throw std::logic_error("Unknown signature algorithm " + std::string(signatureAlgo));
}
