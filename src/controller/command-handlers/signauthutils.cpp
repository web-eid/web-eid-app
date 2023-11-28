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

#include "signauthutils.hpp"

#include "ui.hpp"
#include "commandhandler.hpp"
#include "utils/utils.hpp"

#include <QScopeGuard>

using namespace electronic_id;

// Take argument names by copy/move as they will be modified.
void requireArgumentsAndOptionalLang(QStringList argNames, const QVariantMap& args,
                                     const std::string& argDescriptions)
{
    QVariantMap argCopy {args};
    argCopy.remove(QStringLiteral("lang"));

    // Sorts the list of arguments in ascending order.
    argNames.sort();

    // QMap::keys() also returns a list containing all the keys in the map in ascending order.
    if (argCopy.keys() != argNames) {
        THROW(CommandHandlerInputDataError,
              "Arguments must be '{" + argDescriptions
                  + ", \"lang\": \"<OPTIONAL user interface language>\"}'");
    }
}

template <typename T>
T validateAndGetArgument(const QString& argName, const QVariantMap& args, bool allowNull)
{
    if (!args.contains(argName)) {
        THROW(CommandHandlerInputDataError, "Argument '" + argName.toStdString() + "' is missing");
    }
    if (allowNull && args[argName].isNull()) {
        return args[argName].value<T>();
    }
    auto argValue = args[argName].value<T>();
    if (argValue.isEmpty()) {
        THROW(CommandHandlerInputDataError, "Argument '" + argName.toStdString() + "' is empty");
    }
    return argValue;
}

template QString validateAndGetArgument<QString>(const QString& argName, const QVariantMap& args,
                                                 bool allowNull);
template QByteArray validateAndGetArgument<QByteArray>(const QString& argName,
                                                       const QVariantMap& args, bool allowNull);

template <typename T, typename C>
inline void eraseData(T& data)
{
    // According to docs, constData() never causes a deep copy to occur, so we can abuse it
    // to overwrite the underlying buffer since the underlying data is not really const.
    C* chars = const_cast<C*>(data.constData());
    for (int i = 0; i < data.length(); ++i) {
        chars[i] = '\0';
    }
}

int getPin(pcsc_cpp::byte_vector& pin, const pcsc_cpp::SmartCard& card, WebEidUI* window)
{
    // Doesn't apply to PIN pads.
    if (card.readerHasPinPad()) {
        return 0;
    }

    REQUIRE_NON_NULL(window)

    QString pinqs = window->getPin();
    if (pinqs.isEmpty()) {
        THROW(ProgrammingError, "Empty PIN");
    }
    int len = (int)pinqs.length();

    pin.resize(len);
    for (int i = 0; i < len; i++) {
        pin[i] = pinqs[i].cell();
    }
    eraseData<QString, QChar>(pinqs);

    return len;
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
        THROW(ProgrammingError,
              "Unknown hash algorithm for signature algorithm " + std::string(signatureAlgo));
    }

    if (signatureAlgo & SignatureAlgorithm::ES)
        return {{"cryptoAlgorithm", "ECC"}, {"hashFunction", hashAlgo}, {"paddingScheme", "NONE"}};
    if (signatureAlgo & SignatureAlgorithm::RS)
        return {
            {"cryptoAlgorithm", "RSA"}, {"hashFunction", hashAlgo}, {"paddingScheme", "PKCS1.5"}};
    if (signatureAlgo & SignatureAlgorithm::PS)
        return {{"cryptoAlgorithm", "RSA"}, {"hashFunction", hashAlgo}, {"paddingScheme", "PSS"}};
    THROW(ProgrammingError, "Unknown signature algorithm " + std::string(signatureAlgo));
}
