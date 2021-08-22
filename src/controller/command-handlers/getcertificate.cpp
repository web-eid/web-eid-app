/*
 * Copyright (c) 2020-2021 Estonian Information System Authority
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
    requireArgumentsAndOptionalLang({"origin"}, arguments, "\"origin\": \"<origin URL>\"");
}

QVariantMap GetCertificate::onConfirm(WebEidUI* /* window */,
                                      const CardCertificateAndPinInfo& cardCertAndPin)
{
    // Quoting https://tools.ietf.org/html/rfc7515#section-4.1.6:
    // Each string in the array is a Base64-encoded (Section 4 of [RFC4648] -- not
    // Base64url-encoded) DER [ITU.X690.2008] PKIX certificate value.
    auto certPem = cardCertAndPin.certificateBytesInDer.toBase64();
    auto algos = supportedSigningAlgos(cardCertAndPin.cardInfo->eid());
    return {{"certificate", QString(certPem)}, {"supported-signature-algos", algos}};
}
