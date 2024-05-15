/*
 * Copyright (c) 2020-2024 Estonian Information System Authority
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

#include "retriableerror.hpp"

#include <QDebug>
#include "magic_enum/magic_enum.hpp"

QDebug& operator<<(QDebug& d, const RetriableError e)
{
    return d << QString::fromStdString(std::string(magic_enum::enum_name(e)));
}

RetriableError toRetriableError(const electronic_id::AutoSelectFailed::Reason reason)
{
    using Reason = electronic_id::AutoSelectFailed::Reason;

    switch (reason) {
    case Reason::SERVICE_NOT_RUNNING:
        return RetriableError::SMART_CARD_SERVICE_IS_NOT_RUNNING;
    case Reason::NO_READERS:
        return RetriableError::NO_SMART_CARD_READERS_FOUND;
    case Reason::SINGLE_READER_NO_CARD:
    case Reason::MULTIPLE_READERS_NO_CARD:
        return RetriableError::NO_SMART_CARDS_FOUND;
    case Reason::SINGLE_READER_UNSUPPORTED_CARD:
    case Reason::MULTIPLE_READERS_NO_SUPPORTED_CARD:
        return RetriableError::UNSUPPORTED_CARD;
    }
    return RetriableError::UNKNOWN_ERROR;
}
