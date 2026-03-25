// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "ui.hpp"

/** Interface for command handlers that implement the actual work the application does, like get
 * certificate, authenticate and sign. */
class CommandHandler : public QObject
{
    Q_OBJECT

public:
    using ptr = std::unique_ptr<CommandHandler>;

    virtual void run(std::vector<electronic_id::ElectronicID::ptr>&& eids) = 0;
    virtual void connectSignals(const WebEidUI* window) = 0;
    virtual QVariantMap onConfirm(WebEidUI* window,
                                  const EidCertificateAndPinInfo& certAndPinInfo) = 0;

    CommandType commandType() const { return command.first; }

signals:
    void retry(const RetriableError error);
    void multipleCertificatesReady(const QUrl& origin,
                                   const std::vector<EidCertificateAndPinInfo>& certAndPinInfos);
    void singleCertificateReady(const QUrl& origin, const EidCertificateAndPinInfo& certAndPinInfo);
    void verifyPinFailed(electronic_id::VerifyPinFailed::Status status, qint8 retriesLeft);

protected:
    CommandHandler(const CommandWithArguments& cmd) : command(cmd) {}
    CommandWithArguments command;
};

CommandHandler::ptr getCommandHandler(const CommandWithArguments& cmd);

/** Base class for command handler errors. */
class CommandHandlerError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/** Exception that signals expected invalid PIN entry errors. */
class CommandHandlerVerifyPinFailed : public CommandHandlerError
{
public:
    using CommandHandlerError::CommandHandlerError;
};

/** Exception that signals errors in input data (from stdin or arguments), e.g. invalid origin or
 * certificate. */
class CommandHandlerInputDataError : public CommandHandlerError
{
public:
    using CommandHandlerError::CommandHandlerError;
};
