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

#include "certificatelistwidget.hpp"

#include "ui_dialog.h"

#include <QPushButton>
#include <QTimeLine>
#include <QMessageBox>
#include <QMutexLocker>
#include <QRegularExpressionValidator>
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
        return Page::INSERT_CARD;
    case CommandType::GET_CERTIFICATE:
        return Page::SELECT_CERTIFICATE;
    case CommandType::AUTHENTICATE:
    case CommandType::SIGN:
        return Page::PININPUT;
    default:
        THROW(ProgrammingError, "No page exists for command " + std::string(command));
    }
}

void fillOriginAndCertificateList(
    QLabel* originLabel, CertificateListWidget* certificateWidget, const QUrl& origin,
    const std::vector<CardCertificateAndPinInfo>& certificateAndPinInfos)
{
    originLabel->setText(fromPunycode(origin));
    certificateWidget->setCertificateInfo(certificateAndPinInfos);
}

} // namespace

WebEidDialog::WebEidDialog(QWidget* parent) : WebEidUI(parent), ui(new Ui::WebEidDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowTitleHint);
    ui->pinLayout->setAlignment(ui->pinInput, Qt::AlignCenter);
    ui->pinInput->setAttribute(Qt::WA_MacShowFocusRect, false);

    okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
    makeOkButtonDefaultAndconnectSignals();

    // Hide PIN-related widgets by default.
    ui->pinErrorLabel->hide();
    ui->pinInput->hide();
    ui->pinEntryTimeoutProgressBar->hide();
}

WebEidDialog::~WebEidDialog()
{
    delete ui;
}

void WebEidDialog::showWaitingForCardPage(const CommandType commandType)
{
    currentCommand = commandType;

    // Don't show OK button while waiting for card operation or connect card.
    okButton->hide();

    const auto pageIndex =
        commandType == CommandType::INSERT_CARD ? int(Page::INSERT_CARD) : int(Page::WAITING);
    ui->pageStack->setCurrentIndex(pageIndex);
    switch (commandType) {
    case CommandType::AUTHENTICATE:
        ui->pinInputPageTitleLabel->setText(tr("Authenticate"));
        ui->pinInputDescriptionLabel->setText(
            tr("By confirming authentication, I agree to submit my name and personal "
               "identification number to the website"));
        ui->pinTitleLabel->setText(tr("Please enter authentication PIN (PIN 1):"));
        break;
    case CommandType::SIGN:
        ui->pinInputPageTitleLabel->setText(tr("Sign"));
        ui->pinInputDescriptionLabel->setText(
            tr("By confirming signing, I agree to submit my name and personal identification "
               "number to the website"));
        ui->pinTitleLabel->setText(tr("Please enter signing PIN (PIN 2):"));
        break;
    default:
        break;
    }

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
    ui->selectCardPageTitleLabel->setText(title);
    ui->cardChipIcon->setPixmap(icon);

    // In case the insert card page is not shown, switch back to it.
    okButton->hide();
    showPage(Page::INSERT_CARD);
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
    okButton->setEnabled(false);

    try {
        fillOriginAndCertificateList(ui->selectCertificateOriginLabel, ui->selectCertificateInfo,
                                     origin, certificateAndPinInfos);

        disableOKUntilCertificateSelected(ui->selectCertificateInfo);

        switch (currentCommand) {
        case CommandType::GET_CERTIFICATE:
            connectOkToEmitSelectedCertificate(ui->selectCertificateInfo);
            break;
        case CommandType::AUTHENTICATE:
            // Authenticate continues with the selected certificate to onSingleCertificateReady().
            okButton->disconnect();
            connect(okButton, &QPushButton::clicked, this, [this, origin] {
                onSingleCertificateReady(origin, ui->selectCertificateInfo->selectedCertificate());
            });
            break;
        default:
            THROW(ProgrammingError, "Command " + std::string(currentCommand) + " not allowed here");
        }

        enableAndShowOK();
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
    readerHasPinPad = certAndPin.pinInfo.readerHasPinPad;
    okButton->setEnabled(false);

    try {
        const auto page = commandToPage(currentCommand);
        if (page == Page::INSERT_CARD) {
            THROW(ProgrammingError, "Insert card commmand not allowed here");
        }
        auto [originLabel, certificateWidget] = originLabelAndCertificateListOnPage();

        fillOriginAndCertificateList(originLabel, certificateWidget, origin, {certAndPin});
        certificateWidget->selectFirstRow();

        if (currentCommand == CommandType::GET_CERTIFICATE) {
            connectOkToEmitSelectedCertificate(certificateWidget);
            enableAndShowOK();

        } else if (certAndPin.pinInfo.pinIsBlocked) {
            displayPinBlockedError();

        } else if (certAndPin.pinInfo.readerHasPinPad) {
            setupPinPadProgressBarAndEmitWait();
            displayPinRetriesRemaining(certAndPin.pinInfo.pinRetriesCount);

        } else {
            connectOkToCachePinAndEmitSelectedCertificate(certificateWidget);
            setupPinInputValidator(certAndPin.pinInfo.pinMinMaxLength);
            displayPinRetriesRemaining(certAndPin.pinInfo.pinRetriesCount);

            enableAndShowOK();
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
    try {
        using Status = electronic_id::VerifyPinFailed::Status;

        QString message;

        // FIXME: don't allow retry in case of PIN_BLOCKED, UNKNOWN_ERROR
        switch (status) {
        case Status::RETRY_ALLOWED:
            message = tr("Incorrect PIN, %n retries left", nullptr, retriesLeft);
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

        if (readerHasPinPad.value_or(false)) {
            onRetryImpl(message);
        }
    }
    CATCH_AND_EMIT_FAILURE_AND_RETURN()
}

void WebEidDialog::showPage(const WebEidDialog::Page page)
{
    if (ui->pageStack->currentIndex() != int(page)) {
        ui->pageStack->setCurrentIndex(int(page));
    }
    resizeHeight();
}

void WebEidDialog::makeOkButtonDefaultAndconnectSignals()
{
    connect(cancelButton, &QPushButton::clicked, this, &WebEidDialog::rejected);

    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    okButton->setDefault(true);
    okButton->setAutoDefault(true);
}

void WebEidDialog::connectOkToEmitSelectedCertificate(CertificateListWidget* certificateWidget)
{
    okButton->disconnect();
    connect(okButton, &QPushButton::clicked, this,
            [this, certificateWidget] { emitSelectedCertificate(certificateWidget); });
}

void WebEidDialog::connectOkToCachePinAndEmitSelectedCertificate(
    CertificateListWidget* certificateWidget)
{
    okButton->disconnect();
    connect(okButton, &QPushButton::clicked, this, [this, certificateWidget] {
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

        emitSelectedCertificate(certificateWidget);
    });
}

void WebEidDialog::emitSelectedCertificate(CertificateListWidget* certificateWidget)
{
    try {
        emit accepted(certificateWidget->selectedCertificate());
    }
    CATCH_AND_EMIT_FAILURE_AND_RETURN()
}

void WebEidDialog::onRetryImpl(const QString& error)
{
    showPage(Page::WAITING);

    const auto result =
        QMessageBox::warning(this, tr("Retry?"), tr("Error occurred: ") + error,
                             QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
    emit result == QMessageBox::Yes ? retry() : reject();
}

void WebEidDialog::setupPinPadProgressBarAndEmitWait()
{
    hide();
    setWindowFlag(Qt::WindowCloseButtonHint, false);
    show();

    okButton->hide();
    cancelButton->hide();
    ui->pinEntryTimeoutProgressBar->show();
    ui->pinTitleLabel->setText(tr("Please enter %1 using PIN pad")
                                   .arg(currentCommand == CommandType::AUTHENTICATE
                                            ? tr("authentication PIN (PIN 1)")
                                            : tr("signing PIN (PIN 2)")));

    startPinTimeoutProgressBar();

    auto certificateWidget = originLabelAndCertificateListOnPage().second;
    emit waitingForPinPad(certificateWidget->selectedCertificate());
}

void WebEidDialog::setupPinInputValidator(const PinInfo::PinMinMaxLength& pinMinMaxLength)
{
    try {
        ui->pinInput->setMaxLength(int(pinMinMaxLength.second));

        const auto numericMinMaxRegexp = QRegularExpression(
            QStringLiteral("[0-9]{%1,%2}").arg(pinMinMaxLength.first).arg(pinMinMaxLength.second));
        ui->pinInput->setValidator(
            new QRegularExpressionValidator(numericMinMaxRegexp, ui->pinInput));

        ui->pinInput->disconnect();
        connect(ui->pinInput, &QLineEdit::textChanged, okButton,
                [this] { okButton->setEnabled(ui->pinInput->hasAcceptableInput()); });

        ui->pinInput->show();
        ui->pinInput->setFocus();

        resizeHeight();
    }
    CATCH_AND_EMIT_FAILURE_AND_RETURN()
}

void WebEidDialog::startPinTimeoutProgressBar()
{
    try {
        ui->pinEntryTimeoutProgressBar->reset();
        ui->pinEntryTimeoutProgressBar->setMaximum(PinInfo::PIN_PAD_PIN_ENTRY_TIMEOUT);
        QTimeLine* previousPinTimeoutTimer =
            ui->pinEntryTimeoutProgressBar->findChild<QTimeLine*>();
        if (previousPinTimeoutTimer) {
            previousPinTimeoutTimer->stop();
            previousPinTimeoutTimer->deleteLater();
        }

        QTimeLine* pinTimeoutTimer = new QTimeLine(ui->pinEntryTimeoutProgressBar->maximum() * 1000,
                                                   ui->pinEntryTimeoutProgressBar);
        pinTimeoutTimer->setEasingCurve(QEasingCurve::Linear);
        pinTimeoutTimer->setFrameRange(ui->pinEntryTimeoutProgressBar->minimum(),
                                       ui->pinEntryTimeoutProgressBar->maximum());
        connect(pinTimeoutTimer, &QTimeLine::frameChanged, ui->pinEntryTimeoutProgressBar,
                &QProgressBar::setValue);
        connect(pinTimeoutTimer, &QTimeLine::finished, pinTimeoutTimer, &QTimeLine::deleteLater);

        // To be strictly correct, the timeout timer should be started after the handler thread
        // has triggered the PIN pad internal timeout timer. However, that would involve extra
        // complexity in signal-slot setup that would bring little value as the difference between
        // timers is undetectable to the user, so we simply start the timer here, slightly earlier
        // than the PIN pad timer.
        pinTimeoutTimer->start();
    }
    CATCH_AND_EMIT_FAILURE_AND_RETURN()
}

void WebEidDialog::hidePinWidgets()
{
    // Do nothing in case the PIN widgets are not on the page.
    if (currentCommand != CommandType::AUTHENTICATE && currentCommand != CommandType::SIGN) {
        return;
    }

    ui->pinTitleLabel->hide();
    ui->pinInput->hide();
}

void WebEidDialog::enableAndShowOK()
{
    okButton->setEnabled(true);
    okButton->show();
}

void WebEidDialog::disableOKUntilCertificateSelected(const CertificateListWidget* certificateWidget)
{
    okButton->setEnabled(false);
    certificateWidget->disconnect();
    connect(certificateWidget, &CertificateListWidget::certificateSelected, this,
            [this] { okButton->setEnabled(true); });
}

void WebEidDialog::displayPinRetriesRemaining(PinInfo::PinRetriesCount pinRetriesCount)
{
    if (pinRetriesCount.first != pinRetriesCount.second) {
        ui->pinErrorLabel->setText(tr("%n retries left", nullptr, int(pinRetriesCount.first)));
        ui->pinErrorLabel->show();
    }
}

void WebEidDialog::displayPinBlockedError()
{
    okButton->hide();
    hidePinWidgets();
    ui->pinErrorLabel->setText(tr("PIN is blocked, cannot proceed"));
    ui->pinErrorLabel->show();
}

void WebEidDialog::resizeHeight()
{
    ui->pageStack->setFixedHeight(ui->pageStack->currentWidget()->sizeHint().height());
    adjustSize();
}

std::pair<QLabel*, CertificateListWidget*> WebEidDialog::originLabelAndCertificateListOnPage()
{
    switch (currentCommand) {
    case CommandType::GET_CERTIFICATE:
        return {ui->selectCertificateOriginLabel, ui->selectCertificateInfo};
    case CommandType::AUTHENTICATE:
    case CommandType::SIGN:
        return {ui->pinInputOriginLabel, ui->pinInputCertificateInfo};
    default:
        THROW(ProgrammingError, "Only SELECT_CERTIFICATE, AUTHENTICATE or SIGN allowed");
    }
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
