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

#include "registermetatypes.hpp"

#include "commands.hpp"
#include "certandpininfo.hpp"
#include "retriableerror.hpp"

void registerMetatypes()
{
    // FIXME: only register signal parameter metatypes, too much stuff at the moment
    qRegisterMetaType<electronic_id::AutoSelectFailed::Reason>();
    qRegisterMetaType<electronic_id::CardInfo::ptr>();
    qRegisterMetaType<std::vector<electronic_id::CardInfo::ptr>>();
    qRegisterMetaType<electronic_id::CertificateType>();
    qRegisterMetaType<electronic_id::VerifyPinFailed::Status>();

    qRegisterMetaType<CommandType>();

    qRegisterMetaType<CardCertificateAndPinInfo>();
    qRegisterMetaType<std::vector<CardCertificateAndPinInfo>>();

    qRegisterMetaType<RetriableError>();
}
