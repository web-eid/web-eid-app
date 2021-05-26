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

#include "webeiddialog.hpp"
#include "punycode.hpp"

#include "ui_dialog.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QMutexLocker>
#include <QRegularExpressionValidator>
#include <QTimeLine>
#include <QUrl>

#define CATCH_AND_EMIT_FAILURE_AND_RETURN()                                                        \
    catch (std::exception & e)                                                                     \
    {                                                                                              \
        emit failure(e.what());                                                                    \
        return;                                                                                    \
    }

using namespace electronic_id;

namespace
{

WebEidDialog::Page commandToPage(const CommandType command)
{
    using Page = WebEidDialog::Page;
    switch (command) {
    case CommandType::INSERT_CARD:
        return Page::MESSAGE;
    case CommandType::GET_CERTIFICATE:
        return Page::SELECT_CERTIFICATE;
    case CommandType::AUTHENTICATE:
    case CommandType::SIGN:
        return Page::PININPUT;
    default:
        THROW(ProgrammingError, "No page exists for command " + std::string(command));
    }
}

} // namespace

class WebEidDialog::Private : public Ui::WebEidDialog
{
public:
    // Non-owning observer pointers.
    QRegularExpressionValidator* pinInputValidator;
    QTimeLine* pinTimeoutTimer;
};

WebEidDialog::WebEidDialog(QWidget* parent) : WebEidUI(parent), ui(new Private)
{
    ui->setupUi(this);
    setWindowFlag(Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowTitleHint);
    ui->messageInfoLayout->setAlignment(ui->cardChipIcon, Qt::AlignTop);
    ui->pinLayout->setAlignment(ui->pinInput, Qt::AlignCenter);
    ui->pinInput->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->selectCertificateInfo, &CertificateListWidget::currentItemChanged, this,
            [this] { ui->okButton->setEnabled(true); });
    connect(ui->cancelButton, &QPushButton::clicked, this, &WebEidDialog::rejected);
    connect(ui->helpButton, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(tr("https://www.id.ee/en/")); });

    // Hide PIN-related widgets by default.
    ui->pinErrorLabel->hide();
    ui->pinInput->hide();
    ui->pinEntryTimeoutProgressBar->hide();

    ui->pinInputValidator = new QRegularExpressionValidator(ui->pinInput);
    ui->pinInput->setValidator(ui->pinInputValidator);
    connect(ui->pinInput, &QLineEdit::textChanged, ui->okButton,
            [this] { ui->okButton->setEnabled(ui->pinInput->hasAcceptableInput()); });

    ui->pinEntryTimeoutProgressBar->setMaximum(PinInfo::PIN_PAD_PIN_ENTRY_TIMEOUT);
    ui->pinTimeoutTimer = new QTimeLine(ui->pinEntryTimeoutProgressBar->maximum() * 1000,
                                        ui->pinEntryTimeoutProgressBar);
    ui->pinTimeoutTimer->setEasingCurve(QEasingCurve::Linear);
    ui->pinTimeoutTimer->setFrameRange(ui->pinEntryTimeoutProgressBar->minimum(),
                                       ui->pinEntryTimeoutProgressBar->maximum());
    connect(ui->pinTimeoutTimer, &QTimeLine::frameChanged, ui->pinEntryTimeoutProgressBar,
            &QProgressBar::setValue);
}

WebEidDialog::~WebEidDialog()
{
    delete ui;
}

void WebEidDialog::showWaitingForCardPage(const CommandType commandType)
{
    currentCommand = commandType;

    // Don't show OK button while waiting for card operation or connect card.
    ui->okButton->hide();

    const auto pageIndex =
        commandType == CommandType::INSERT_CARD ? int(Page::MESSAGE) : int(Page::WAITING);
    ui->pageStack->setCurrentIndex(pageIndex);
    resizeHeight();
}

QString WebEidDialog::getPin()
{
    // getPin() is called from background threads and must be thread-safe.
    // QString uses QAtomicPointer internally and is thread-safe.
    return pin;
}

void WebEidDialog::onSmartCardStatusUpdate(const RetriableError status)
{
    const auto [errorText, title, icon] = retriableErrorToTextTitleAndIcon(status);

    ui->connectCardLabel->setText(errorText);
    ui->messagePageTitleLabel->setText(title);
    ui->cardChipIcon->setPixmap(icon);

    // In case the insert card page is not shown, switch back to it.
    ui->helpButton->show();
    ui->cancelButton->show();
    ui->okButton->hide();
    showPage(Page::MESSAGE);
}

/** This slot is used by the get certificate and authenticate commands in case there are multiple
 * certificates available. It displays the certificate selection view with multiple certificates.
 *
 * Get certificate exits the flow on OK with the selected certificate from here,
 * authenticate continues to onSingleCertificateReady().
 */
void WebEidDialog::onMultipleCertificatesReady(
    const QUrl& origin, const std::vector<CardCertificateAndPinInfo>& certificateAndPinInfos)
{
    try {
        ui->selectCertificateOriginLabel->setText(fromPunycode(origin));
        ui->selectCertificateInfo->setCertificateInfo(certificateAndPinInfos);

        switch (currentCommand) {
        case CommandType::GET_CERTIFICATE:
            setupOK([this] {
                try {
                    emit accepted(ui->selectCertificateInfo->selectedCertificate());
                }
                CATCH_AND_EMIT_FAILURE_AND_RETURN()
            });

            break;
        case CommandType::AUTHENTICATE:
            // Authenticate continues with the selected certificate to onSingleCertificateReady().
            setupOK([this, origin] {
                try {
                    onSingleCertificateReady(origin,
                                             ui->selectCertificateInfo->selectedCertificate());
                }
                CATCH_AND_EMIT_FAILURE_AND_RETURN()
            });
            break;
        default:
            THROW(ProgrammingError, "Command " + std::string(currentCommand) + " not allowed here");
        }

        showPage(Page::SELECT_CERTIFICATE);
    }
    CATCH_AND_EMIT_FAILURE_AND_RETURN()
}

/** This slot is used by all commands in case there is only a single certificate available. It
 * displays the certificate confirmation view and, in case of authenticate or sign, the PIN input
 * widgets.
 *
 * Authenticate enters here also from onCertificatesReady() after a certificate has been selected.
 *
 * All of the commands exit the flow on OK with the selected certificate from here.
 */
void WebEidDialog::onSingleCertificateReady(const QUrl& origin,
                                            const CardCertificateAndPinInfo& certAndPin)
{
    try {
        const auto page = commandToPage(currentCommand);
        if (page == Page::MESSAGE) {
            THROW(ProgrammingError, "Insert card commmand not allowed here");
        }
        switch (currentCommand) {
        case CommandType::GET_CERTIFICATE:
            ui->selectCertificateInfo->setCertificateInfo({certAndPin});
            break;
        case CommandType::AUTHENTICATE:
            ui->pinInputCertificateInfo->setCertificateInfo(certAndPin);
            ui->pinInputPageTitleLabel->setText(tr("Authenticate"));
            ui->pinInputDescriptionLabel->setText(
                tr("By confirming authentication, I agree to submit my name and personal "
                   "identification number to the website"));
            ui->pinTitleLabel->setText(tr("Please enter authentication PIN (PIN 1):"));
        case CommandType::SIGN:
            ui->pinInputCertificateInfo->setCertificateInfo(certAndPin);
            ui->pinInputPageTitleLabel->setText(tr("Sign"));
            ui->pinInputDescriptionLabel->setText(
                tr("By confirming signing, I agree to submit my name and personal identification "
                   "number to the website"));
            ui->pinTitleLabel->setText(tr("Please enter signing PIN (PIN 2):"));
            break;
        default:
            THROW(ProgrammingError, "Only SELECT_CERTIFICATE, AUTHENTICATE or SIGN allowed");
        }
        ui->selectCertificateOriginLabel->setText(fromPunycode(origin));
        ui->pinInputOriginLabel->setText(ui->selectCertificateOriginLabel->text());

        if (currentCommand == CommandType::GET_CERTIFICATE) {
            setupOK([this, certAndPin] { emit accepted(certAndPin); });

        } else if (certAndPin.pinInfo.pinIsBlocked) {
            displayPinBlockedError();

        } else if (certAndPin.pinInfo.readerHasPinPad) {
            setupPinPadProgressBarAndEmitWait(certAndPin);
            displayPinRetriesRemaining(certAndPin.pinInfo.pinRetriesCount);

        } else {
            connectOkToCachePinAndEmitSelectedCertificate(certAndPin);
            setupPinInputValidator(certAndPin.pinInfo.pinMinMaxLength);
            displayPinRetriesRemaining(certAndPin.pinInfo.pinRetriesCount);
        }

        showPage(page);
    }
    CATCH_AND_EMIT_FAILURE_AND_RETURN()
}

void WebEidDialog::onRetry(const RetriableError error)
{
    onRetryImpl(std::get<0>(retriableErrorToTextTitleAndIcon(error)));
}

void WebEidDialog::onCertificateNotFound(const QString& certificateSubject)
{
    onRetryImpl(tr("No electronic ID card is inserted that has the signing certificate provided as "
                   "argument. Please insert the electronic ID card that belongs to %1")
                    .arg(certificateSubject));
}

void WebEidDialog::onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                                     const quint8 retriesLeft)
{
    using Status = electronic_id::VerifyPinFailed::Status;

    QString message;

    // FIXME: don't allow retry in case of PIN_BLOCKED, UNKNOWN_ERROR
    switch (status) {
    case Status::RETRY_ALLOWED:
        message = tr("Incorrect PIN, %n retries left", nullptr, retriesLeft);
        style()->unpolish(ui->pinInput);
        ui->pinInput->setProperty("warning", true);
        style()->polish(ui->pinInput);
        break;
    case Status::PIN_BLOCKED:
        message = tr("PIN blocked");
        break;
    case Status::INVALID_PIN_LENGTH:
        message = tr("Wrong PIN length");
        break;
    case Status::PIN_ENTRY_TIMEOUT:
        message = tr("PIN pad PIN entry timeout");
        break;
    case Status::PIN_ENTRY_CANCEL:
        message = tr("PIN pad PIN entry cancelled");
        break;
    case Status::UNKNOWN_ERROR:
        message = tr("Technical error");
        break;
    }

    ui->pinErrorLabel->setHidden(message.isEmpty());
    ui->pinErrorLabel->setText(message);

    if (ui->pinEntryTimeoutProgressBar->isVisible()) {
        onRetryImpl(message);
    } else {
        ui->pinInput->show();
        ui->pinTitleLabel->show();
        ui->okButton->setEnabled(true);
        ui->cancelButton->setEnabled(true);
    }

    resizeHeight();
}

void WebEidDialog::reject()
{
    if (!ui->pinEntryTimeoutProgressBar->isVisible()) {
        WebEidUI::reject();
    }
}

bool WebEidDialog::event(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    return WebEidUI::event(event);
}

void WebEidDialog::showPage(const WebEidDialog::Page page)
{
    if (ui->pageStack->currentIndex() != int(page)) {
        ui->pageStack->setCurrentIndex(int(page));
        resizeHeight();
    }
}

void WebEidDialog::connectOkToCachePinAndEmitSelectedCertificate(
    const CardCertificateAndPinInfo& certAndPin)
{
    setupOK([this, certAndPin] {
        ui->pinInput->hide();
        ui->pinTitleLabel->hide();
        ui->pinErrorLabel->hide();
        ui->okButton->setDisabled(true);
        ui->cancelButton->setDisabled(true);
        // Cache the PIN in an instance variable for later use in getPin().
        // This is required as accessing widgets from background threads is not allowed,
        // so getPin() cannot access pinInput directly.
        // QString uses QAtomicPointer internally and is thread-safe.
        pin = ui->pinInput->text();

        // TODO: We need to erase the PIN in the widget buffer, this needs further work.
        // Investigate if it is possible to keep the PIN in secure memory, e.g. with a
        // custom Qt widget.
        // Clear the PIN input.
        ui->pinInput->setText({});

        emit accepted(certAndPin);
    });
}

void WebEidDialog::onRetryImpl(const QString& error)
{
    ui->connectCardLabel->setText(error);
    ui->messagePageTitleLabel->setText(tr("Error occurred"));
    ui->cardChipIcon->setPixmap(QStringLiteral(":/images/id-card.svg"));
    showPage(Page::MESSAGE);
    setupOK([this] { emit retry(); }, tr("Retry"), true);
}

void WebEidDialog::setupPinPadProgressBarAndEmitWait(const CardCertificateAndPinInfo& certAndPin)
{
    hide();
    setWindowFlag(Qt::WindowCloseButtonHint, false);
    show();

    ui->okButton->hide();
    ui->cancelButton->hide();
    ui->helpButton->hide();
    ui->pinEntryTimeoutProgressBar->show();
    ui->pinTitleLabel->setText(tr("Please enter %1 using PIN pad")
                                   .arg(currentCommand == CommandType::AUTHENTICATE
                                            ? tr("authentication PIN (PIN 1)")
                                            : tr("signing PIN (PIN 2)")));

    ui->pinEntryTimeoutProgressBar->reset();
    // To be strictly correct, the timeout timer should be started after the handler thread
    // has triggered the PIN pad internal timeout timer. However, that would involve extra
    // complexity in signal-slot setup that would bring little value as the difference between
    // timers is undetectable to the user, so we simply start the timer here, slightly earlier
    // than the PIN pad timer.
    ui->pinTimeoutTimer->start();

    emit waitingForPinPad(certAndPin);
}

void WebEidDialog::setupPinInputValidator(const PinInfo::PinMinMaxLength& pinMinMaxLength)
{
    const auto numericMinMaxRegexp = QRegularExpression(
        QStringLiteral("[0-9]{%1,%2}").arg(pinMinMaxLength.first).arg(pinMinMaxLength.second));
    ui->pinInputValidator->setRegularExpression(numericMinMaxRegexp);
    ui->pinInput->setMaxLength(int(pinMinMaxLength.second));
    ui->pinInput->show();
    ui->pinInput->setFocus();
    resizeHeight();
}

void WebEidDialog::setupOK(const std::function<void()>& func, const QString& label, bool enabled)
{
    ui->okButton->disconnect();
    connect(ui->okButton, &QPushButton::clicked, this, func);
    ui->okButton->show();
    ui->okButton->setEnabled(enabled);
    ui->okButton->setText(label.isEmpty() ? tr("Proceed") : label);
    ui->cancelButton->show();
    ui->cancelButton->setEnabled(true);
    ui->helpButton->hide();
}

void WebEidDialog::displayPinRetriesRemaining(PinInfo::PinRetriesCount pinRetriesCount)
{
    style()->unpolish(ui->pinInput);
    ui->pinInput->setProperty("warning", QVariant(pinRetriesCount.first != pinRetriesCount.second));
    style()->polish(ui->pinInput);
    if (pinRetriesCount.first != pinRetriesCount.second) {
        ui->pinErrorLabel->setText(tr("%n retries left", nullptr, int(pinRetriesCount.first)));
        ui->pinErrorLabel->show();
    }
}

void WebEidDialog::displayPinBlockedError()
{
    ui->okButton->hide();
    ui->pinTitleLabel->hide();
    ui->pinInput->hide();
    ui->pinErrorLabel->setText(tr("PIN is blocked, cannot proceed"));
    ui->pinErrorLabel->show();
}

void WebEidDialog::resizeHeight()
{
    ui->pageStack->setFixedHeight(ui->pageStack->currentWidget()->sizeHint().height());
    adjustSize();
}

std::tuple<QString, QString, QString>
WebEidDialog::retriableErrorToTextTitleAndIcon(const RetriableError error)
{
    switch (error) {
    case RetriableError::SMART_CARD_SERVICE_IS_NOT_RUNNING:
        return {tr("Smart card service is not running. Please start it."),
                tr("Start smart card service"), QStringLiteral(":/images/cardreader.svg")};
    case RetriableError::NO_SMART_CARD_READERS_FOUND:
        return {tr("No readers attached. Please connect a smart card reader."),
                tr("Connect a smart card reader"), QStringLiteral(":/images/cardreader.svg")};

    case RetriableError::NO_SMART_CARDS_FOUND:
    case RetriableError::PKCS11_TOKEN_NOT_PRESENT:
        return {tr("No smart card in reader. "
                   "Please insert an electronic ID card into the reader."),
                tr("Insert an ID card"), QStringLiteral(":/images/id-card.svg")};
    case RetriableError::SMART_CARD_WAS_REMOVED:
    case RetriableError::PKCS11_TOKEN_REMOVED:
        return {tr("The smart card was removed. "
                   "Please insert an electronic ID card into the reader."),
                tr("Insert an ID card"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::SMART_CARD_TRANSACTION_FAILED:
        return {tr("The smart card transaction failed. "
                   "Please make sure that the smart card and reader are properly connected."),
                tr("Check the ID card connection"), QStringLiteral(":/images/id-card.svg")};
    case RetriableError::FAILED_TO_COMMUNICATE_WITH_CARD_OR_READER:
        return {tr("Failed to communicate with the smart card or reader. "
                   "Please make sure that the smart card and reader are properly connected."),
                tr("Check the ID card connection"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::SMART_CARD_CHANGE_REQUIRED:
        return {tr("The smart card is malfunctioning, please change the smart card."),
                tr("Change the ID card"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::SMART_CARD_COMMAND_ERROR:
        return {tr("A smart card command failed."), tr("ID card failure"),
                QStringLiteral(":/images/id-card.svg")};
        // TODO: what action should the user take? Should this be fatal?
    case RetriableError::PKCS11_ERROR:
        return {tr("Smart card middleware error."), tr("ID card middleware failure"),
                QStringLiteral(":/images/id-card.svg")};
        // TODO: what action should the user take? Should this be fatal?
    case RetriableError::SCARD_ERROR:
        return {tr("Internal smart card service error occurred. "
                   "Please make sure that the smart card and reader are properly connected "
                   "or try restarting the smart card service."),
                tr("ID card failure"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::UNSUPPORTED_CARD:
        return {tr("Unsupported smart card in reader. "
                   "Please insert a supported electronic ID card into the reader."),
                tr("Change the ID card"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::NO_VALID_CERTIFICATE_AVAILABLE:
        return {tr("No ID card with valid certificate available. Please insert "
                   "an ID card that has a valid certificate."),
                tr("Change the ID card"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::UNKNOWN_ERROR:
        return {tr("Unknown error"), tr("Unknown error"), QStringLiteral(":/images/id-card.svg")};
    }
    return {tr("Unknown error"), tr("Unknown error"), QStringLiteral(":/images/id-card.svg")};
}
