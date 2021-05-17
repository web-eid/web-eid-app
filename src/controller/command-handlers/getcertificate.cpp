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

#include "getcertificate.hpp"

#include "signauthutils.hpp"

using namespace electronic_id;

namespace
{

QVariantList supportedAuthAlgo(const ElectronicID& eid)
{
    switch (eid.authSignatureAlgorithm()) {
    case JsonWebSignatureAlgorithm::ES256:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::ES256)};
    case JsonWebSignatureAlgorithm::ES384:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::ES384)};
    case JsonWebSignatureAlgorithm::ES512:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::ES512)};
    case JsonWebSignatureAlgorithm::PS256:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::PS256)};
    case JsonWebSignatureAlgorithm::PS384:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::PS384)};
    case JsonWebSignatureAlgorithm::PS512:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::PS512)};
    case JsonWebSignatureAlgorithm::RS256:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::RS256)};
    case JsonWebSignatureAlgorithm::RS384:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::RS384)};
    case JsonWebSignatureAlgorithm::RS512:
        return {signatureAlgoToVariantMap(SignatureAlgorithm::RS512)};
    default:
        THROW(ProgrammingError,
              "Unknown authentication signature algorithm "
                  + std::string(eid.authSignatureAlgorithm()));
    }
}

QVariantList supportedSigningAlgos(const ElectronicID& eid)
{
    QVariantList algos;
    for (const SignatureAlgorithm& signAlgo : eid.supportedSigningAlgorithms()) {
        algos.push_back(signatureAlgoToVariantMap(signAlgo));
    }
    return algos;
}

} // namespace

GetCertificate::GetCertificate(const CommandWithArguments& cmd) : CertificateReader(cmd)
{
    const auto arguments = cmd.second;
    if (arguments.size() < 2 || arguments.size() > 3) {
        THROW(CommandHandlerInputDataError,
              "Argument must be '{\"type\": [\"auth\"|\"sign\"], \"origin\": \"<origin URL>\"}'");
    }
    if (arguments[QStringLiteral("type")] != QStringLiteral("auth")
        && arguments[QStringLiteral("type")] != QStringLiteral("sign")) {
        THROW(CommandHandlerInputDataError, "Argument type must be either 'auth' or 'sign'");
    }
}

QVariantMap GetCertificate::onConfirm(WebEidUI* /* window */,
                                      const CardCertificateAndPinInfo& cardCertAndPin)
{
    // Quoting https://tools.ietf.org/html/rfc7515#section-4.1.6:
    // Each string in the array is a Base64-encoded (Section 4 of [RFC4648] -- not
    // Base64url-encoded) DER [ITU.X690.2008] PKIX certificate value.
    auto certPem = cardCertAndPin.certificateBytesInDer.toBase64();

    auto algos = certificateType.isAuthentication()
        ? supportedAuthAlgo(cardCertAndPin.cardInfo->eid())
        : supportedSigningAlgos(cardCertAndPin.cardInfo->eid());

    return {{"certificate", QString(certPem)}, {"supported-signature-algos", algos}};
}
