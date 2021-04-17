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

#include <QPushButton>
#include <QTimeLine>
#include <QMessageBox>
#include <QMutexLocker>
#include <QRegularExpressionValidator>
#include <QUrl>

WebEidDialog::WebEidDialog(QWidget* parent) : WebEidUI(parent), ui(new Ui::WebEidDialog)
{
    ui->setupUi(this);
    ui->authenticatePinLayout->setAlignment(ui->authenticationPinInput, Qt::AlignCenter);
    ui->signingPinLayout->setAlignment(ui->signingPinInput, Qt::AlignCenter);
    ui->authenticationPinInput->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->signingPinInput->setAttribute(Qt::WA_MacShowFocusRect, false);

    okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    makeOkButtonDefaultRemoveIconsAndconnectSignals();

    lineHeight = ui->authenticateOriginLabel->height();

    // Hide PIN-related widgets by default.
    ui->authenticationPinErrorLabel->hide();
    ui->signingPinErrorLabel->hide();
    ui->authenticationPinInput->hide();
    ui->signingPinInput->hide();
    ui->authenticatePinEntryTimeoutProgressBar->hide();
    ui->signingPinEntryTimeoutProgressBar->hide();
}

WebEidDialog::~WebEidDialog()
{
    delete ui;
}

void WebEidDialog::onOkButtonClicked()
{
    if (currentCommand == CommandType::AUTHENTICATE || currentCommand == CommandType::SIGN) {
        auto pinInput = pinInputOnPage();

        // Cache the PIN in an instance variable for later use in getPin().
        // This is required as accessing widgets from background threads is not allowed,
        // so getPin() cannot access pinInput directly.
        // QString uses QAtomicPointer internally and is thread-safe.
        pin = pinInput->text();

        // TODO: We need to erase the PIN in the widget buffer, this needs further work.
        // Investigate if it is possible to keep the PIN in secure memory, e.g. with a
        // custom Qt widget.
        // Clear the PIN input.
        pinInput->setText({});
    }

    const auto certificateWidget = std::get<2>(certificateLabelsOnPage());
    emit accepted(certificateWidget->selectedCertificateIndex());
}

void WebEidDialog::switchPage(const CommandType commandType)
{
    currentCommand = commandType;

    // Don't show OK button on connect smart card page.
    okButton->setHidden(commandType == CommandType::INSERT_CARD);

    ui->pageStack->setCurrentIndex(int(commandType));
    resizeHeight();
}

QString WebEidDialog::getPin()
{
    // getPin() is called from background threads and must be thread-safe.
    // QString uses QAtomicPointer internally and is thread-safe.
    return pin;
}

void WebEidDialog::onReaderMonitorStatusUpdate(const RetriableError status)
{
    const auto [errorText, title, icon] = retriableErrorToTextTitleAndIcon(status);

    ui->connectCardLabel->setText(errorText);
    ui->selectCardPageTitleLabel->setText(title);
    ui->cardChipIcon->setPixmap(icon);
}

void WebEidDialog::onCertificatesReady(
    const QUrl& origin, const std::vector<CertificateAndPinInfo>& certificateAndPinInfos)
{
    auto certAndPin = certificateAndPinInfos[0]; // FIXME
    readerHasPinPad = certAndPin.pinInfo.readerHasPinPad;

    auto [descriptionLabel, originLabel, certificateWidget] = certificateLabelsOnPage();

    certificateWidget->setCertificateInfo(certificateAndPinInfos);
    originLabel->setText(fromPunycode(origin));

    // FIXME: figure out a way to handle errors in list
    if (certAndPin.pinInfo.pinRetriesCount.first == 0) {
        displayFatalError(descriptionLabel, tr("PIN is blocked, cannot proceed"));
    } else if (certAndPin.certStatus != CertificateStatus::VALID) {
        QString certStatusStr;
        switch (certAndPin.certStatus) {
        case CertificateStatus::NOT_YET_ACTIVE:
            certStatusStr = tr("Not yet active");
            break;
        case CertificateStatus::EXPIRED:
            certStatusStr = tr("Expired");
            break;
        case CertificateStatus::VALID:
            certStatusStr = tr("Valid");
            break;
        case CertificateStatus::INVALID:
            certStatusStr = tr("Invalid");
            break;
        }
        displayFatalError(descriptionLabel,
                          tr("Certificate is %1, cannot proceed").arg(certStatusStr));

    } else if (currentCommand != CommandType::GET_CERTIFICATE) {
        setupPinInputValidator(certAndPin.pinInfo.pinMinMaxLength);
        if (certAndPin.pinInfo.pinRetriesCount.first != certAndPin.pinInfo.pinRetriesCount.second) {
            auto pinErrorLabel = pinErrorLabelOnPage();
            pinErrorLabel->show();
            pinErrorLabel->setText(
                tr("%n retries left", nullptr, int(certAndPin.pinInfo.pinRetriesCount.first)));
        }
    }
}

void WebEidDialog::onSigningCertificateHashMismatch(const QString& certificateSubject)
{
    // FIXME: this needs also more thought
    onRetryImpl(tr("The signature certificate from the electronic ID card does not match the "
                   "certificate provided as argument. Please insert the electronic ID card "
                   "that belongs to %1")
                    .arg(certificateSubject));
}

void WebEidDialog::onRetry(const RetriableError error)
{
    onRetryImpl(std::get<0>(retriableErrorToTextTitleAndIcon(error)));
}

void WebEidDialog::onRetryImpl(const QString& error)
{
    const auto result =
        QMessageBox::warning(this, tr("Retry?"), tr("Error occurred: ") + error,
                             QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
    if (result == QMessageBox::Yes) {
        if (readerHasPinPad.value_or(false)) {
            startPinTimeoutProgressBar();
        }
        emit retry();
    } else {
        emit reject();
    }
}

void WebEidDialog::resizeHeight()
{
    ui->pageStack->setFixedHeight(ui->pageStack->currentWidget()->sizeHint().height());
    adjustSize();
}

// Returns a tuple of error message, and title and icon.
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

    case RetriableError::MULTIPLE_SUPPORTED_CARDS:
        // TODO: Here we should display a multi-select prompt instead.
        return {tr("Multiple electronic ID cards inserted. Please assure that "
                   "only a single ID card is inserted."),
                tr("Change the ID card"), QStringLiteral(":/images/id-card.svg")};

    case RetriableError::UNKNOWN_ERROR:
        return {tr("Unknown error"), tr("Unknown error"), QStringLiteral(":/images/id-card.svg")};
    }
    return {tr("Unknown error"), tr("Unknown error"), QStringLiteral(":/images/id-card.svg")};
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

    auto pinErrorLabel = pinErrorLabelOnPage();
    pinErrorLabel->setHidden(message.isEmpty());
    pinErrorLabel->setText(message);

    if (readerHasPinPad.value_or(false)) {
        onRetryImpl(message);
    }
}

void WebEidDialog::makeOkButtonDefaultRemoveIconsAndconnectSignals()
{
    auto cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);

    connect(okButton, &QPushButton::clicked, this, &WebEidDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &WebEidDialog::rejected);

    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    okButton->setDefault(true);
    okButton->setAutoDefault(true);

    cancelButton->setIcon({});
    okButton->setIcon({});
}

void WebEidDialog::setupPinInputValidator(const PinInfo::PinMinMaxLength& pinMinMaxLenght)
{
    // Do nothing in case the PIN widgets are not on the page.
    if (currentCommand != CommandType::AUTHENTICATE && currentCommand != CommandType::SIGN) {
        return;
    }

    // OK button will either be hidden when using a PIN pad or otherwise enabled when PIN input min
    // lenght is filled in QLineEdit::textChanged event handler below.
    okButton->setEnabled(false);

    if (readerHasPinPad.value_or(false)) {
        okButton->hide();

        pinEntryTimeoutProgressBarOnPage()->show();

        auto pinTitleLabel = pinTitleLabelOnPage();
        // FIXME: translation
        const auto text = pinTitleLabel->text().replace(':', QStringLiteral(" using PIN-pad"));
        pinTitleLabel->setText(text);

        startPinTimeoutProgressBar();
        // FIXME: PIN pad handling needs more thought
        emit waitingForPinPad(0);

    } else {
        auto pinInput = pinInputOnPage();

        pinInput->show();
        pinInput->setMaxLength(int(pinMinMaxLenght.second));

        const auto numericMinMaxRegexp = QRegularExpression(
            QStringLiteral("[0-9]{%1,%2}").arg(pinMinMaxLenght.first).arg(pinMinMaxLenght.second));
        pinInput->setValidator(new QRegularExpressionValidator(numericMinMaxRegexp, pinInput));

        connect(pinInput, &QLineEdit::textChanged, okButton,
                [this, pinInput] { okButton->setEnabled(pinInput->hasAcceptableInput()); });

        pinInput->setFocus();
    }

    resizeHeight();
}

void WebEidDialog::startPinTimeoutProgressBar()
{
    auto pinTimeoutProgressBar = pinEntryTimeoutProgressBarOnPage();

    pinTimeoutProgressBar->reset();
    pinTimeoutProgressBar->setMaximum(PinInfo::PIN_PAD_PIN_ENTRY_TIMEOUT);
    QTimeLine* previousPinTimeoutTimer = pinTimeoutProgressBar->findChild<QTimeLine*>();
    if (previousPinTimeoutTimer) {
        previousPinTimeoutTimer->stop();
        previousPinTimeoutTimer->deleteLater();
    }

    QTimeLine* pinTimeoutTimer =
        new QTimeLine(pinTimeoutProgressBar->maximum() * 1000, pinTimeoutProgressBar);
    pinTimeoutTimer->setEasingCurve(QEasingCurve::Linear);
    pinTimeoutTimer->setFrameRange(pinTimeoutProgressBar->minimum(),
                                   pinTimeoutProgressBar->maximum());
    connect(pinTimeoutTimer, &QTimeLine::frameChanged, pinTimeoutProgressBar,
            &QProgressBar::setValue);
    connect(pinTimeoutTimer, &QTimeLine::finished, pinTimeoutTimer, &QTimeLine::deleteLater);

    // To be strictly correct, the timeout timer should be started after the handler thread
    // has triggered the PIN pad internal timeout timer. However, that would involve extra
    // complexity in signal-slot setup that would bring little value as the difference between
    // timers is undetectable to the user, so we simply start the timer here, slightly earlier
    // than the PIN pad timer.
    pinTimeoutTimer->start();
}

std::tuple<QLabel*, QLabel*, CertificateWidget*> WebEidDialog::certificateLabelsOnPage()
{
    switch (currentCommand) {
    case CommandType::GET_CERTIFICATE:
        return {ui->selectCertificateDescriptionLabel, ui->selectCertificateOriginLabel,
                ui->selectCertificateInfo};
    case CommandType::AUTHENTICATE:
        return {ui->authenticateDescriptionLabel, ui->authenticateOriginLabel,
                ui->authenticationCertificateInfo};
    case CommandType::SIGN:
        return {ui->signDescriptionLabel, ui->signOriginLabel, ui->signingCertificateInfo};
    default:
        throw std::logic_error("WebEidDialog::certificateLabelsOnPage() applies only to "
                               "GET_CERTIFICATE, AUTHENTICATE or SIGN");
    }
}

QLabel* WebEidDialog::pinErrorLabelOnPage()
{
    switch (currentCommand) {
    case CommandType::AUTHENTICATE:
        return ui->authenticationPinErrorLabel;
    case CommandType::SIGN:
        return ui->signingPinErrorLabel;
    default:
        throw std::logic_error("WebEidDialog::pinErrorLabelOnPage() applies only to "
                               "AUTHENTICATE or SIGN");
    }
}

QLabel* WebEidDialog::pinTitleLabelOnPage()
{
    switch (currentCommand) {
    case CommandType::AUTHENTICATE:
        return ui->authenticationPinTitleLabel;
    case CommandType::SIGN:
        return ui->signingPinTitleLabel;
    default:
        throw std::logic_error("WebEidDialog::pinTitleLabelOnPage() applies only to "
                               "AUTHENTICATE or SIGN");
    }
}

QLineEdit* WebEidDialog::pinInputOnPage()
{
    switch (currentCommand) {
    case CommandType::AUTHENTICATE:
        return ui->authenticationPinInput;
    case CommandType::SIGN:
        return ui->signingPinInput;
    default:
        throw std::logic_error("WebEidDialog::pinInputOnPage() applies only to "
                               "AUTHENTICATE or SIGN");
    }
}

QProgressBar* WebEidDialog::pinEntryTimeoutProgressBarOnPage()
{
    switch (currentCommand) {
    case CommandType::AUTHENTICATE:
        return ui->authenticatePinEntryTimeoutProgressBar;
    case CommandType::SIGN:
        return ui->signingPinEntryTimeoutProgressBar;
    default:
        throw std::logic_error("WebEidDialog::pinEntryTimeoutProgressBarOnPage() applies only to "
                               "AUTHENTICATE or SIGN");
    }
}

void WebEidDialog::displayFatalError(QLabel* descriptionLabel, const QString& message)
{
    okButton->setEnabled(false);
    hidePinAndDocHashWidgets();
    descriptionLabel->setStyleSheet(QStringLiteral("color: darkred"));
    descriptionLabel->setText(message);
}

void WebEidDialog::hidePinAndDocHashWidgets()
{
    // Do nothing in case the PIN widgets are not on the page.
    if (currentCommand != CommandType::AUTHENTICATE && currentCommand != CommandType::SIGN) {
        return;
    }

    pinTitleLabelOnPage()->hide();
    pinInputOnPage()->hide();
}
