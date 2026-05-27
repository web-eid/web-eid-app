// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

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
                                      const EidCertificateAndPinInfo& certAndPinInfo)
{
    // Quoting https://tools.ietf.org/html/rfc7515#section-4.1.6:
    // Each string in the array is a Base64-encoded (Section 4 of [RFC4648] -- not
    // Base64url-encoded) DER [ITU.X690.2008] PKIX certificate value.
    auto certPem = certAndPinInfo.certificateBytesInDer.toBase64();
    auto algos = supportedSigningAlgos(*certAndPinInfo.eid);
    return {{"certificate", QString(certPem)}, {"supportedSignatureAlgorithms", algos}};
}
