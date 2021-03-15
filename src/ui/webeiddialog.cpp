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

namespace
{

void makeLabelForegroundDarkRed(QLabel* label)
{
    label->setStyleSheet("color: darkred");
}

} // namespace

WebEidDialog::WebEidDialog(QWidget* parent) : WebEidUI(parent), ui(new Ui::WebEidDialog)
{
    ui->setupUi(this);

    okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    makeOkButtonDefaultAndconnectSignals();

    lineHeight = ui->authenticateOriginLabel->height();

    // Hide document hash fingerprint label and PIN-related widgets by default.
    ui->docHashFingerprintLabel->hide();
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
        pin = pinInput->text();

        // TODO: We need to erase the PIN in the widget buffer, this needs further work.
        // Investigate if it is possible to keep the PIN in secure memory, e.g. with a
        // custom Qt widget.
        // Clear the PIN input.
        pinInput->setText(QString());
    }

    emit accepted();
}

void WebEidDialog::switchPage(const CommandType commandType)
{
    currentCommand = commandType;

    // Don't show OK button on connect smart card page.
    okButton->setHidden(commandType == CommandType::INSERT_CARD);

    ui->pageStack->setCurrentIndex(int(commandType));
}

QString WebEidDialog::getPin()
{
    // getPin() is called from background threads and must be thread-safe.
    static QMutex mutex;
    QMutexLocker lock {&mutex};

    return pin;
}

void WebEidDialog::onReaderMonitorStatusUpdate(const electronic_id::AutoSelectFailed::Reason status)
{
    using Reason = electronic_id::AutoSelectFailed::Reason;

    switch (status) {
    /** FIXME: SCARD_ERROR must be replaced by specific errors + a generic catch. */
    case Reason::SCARD_ERROR:
        ui->smartCardErrorLabel->setText(tr("Internal smart card service error occurred. "
                                            "Please try restarting the smart card service."));
        break;
    case Reason::SERVICE_NOT_RUNNING:
        ui->smartCardErrorLabel->setText(tr("Smart card service is not running. Please start it."));
        break;
    case Reason::NO_READERS:
        ui->smartCardErrorLabel->setText(tr("No readers attached. "
                                            "Please connect a smart card reader."));
        break;
    case Reason::SINGLE_READER_NO_CARD:
    case Reason::MULTIPLE_READERS_NO_CARD:
        // TODO: Handle unavailable vs no card separately.
        ui->smartCardErrorLabel->setText(
            tr("No smart card in reader. "
               "Please insert an electronic ID card into the reader."));
        break;
    case Reason::SINGLE_READER_UNSUPPORTED_CARD:
    case Reason::MULTIPLE_READERS_NO_SUPPORTED_CARD:
        ui->smartCardErrorLabel->setText(
            tr("Unsupported card in reader. "
               "Please insert an electronic ID card into the reader."));
        break;
    case Reason::MULTIPLE_SUPPORTED_CARDS:
        // TODO: Here we should display a multi-select prompt instead.
        ui->smartCardErrorLabel->setText(
            tr("Multiple electronic ID cards inserted. Please assure that "
               "only single ID card is inserted."));
        break;
    }

    if (ui->spinnerLabel->text() == "....") {
        ui->spinnerLabel->clear();
    } else {
        ui->spinnerLabel->setText(ui->spinnerLabel->text() += '.');
    }
}

void WebEidDialog::onCertificateReady(const QUrl& origin, const CertificateStatus certStatus,
                                      const CertificateInfo& certInfo, const PinInfo& pinInfo)
{
    readerHasPinPad = pinInfo.readerHasPinPad;

    auto [descriptionLabel, originLabel, certInfoLabel, icon] = certificateLabelsOnPage();

    const auto certType = certInfo.type.isAuthentication() ? QStringLiteral("Authentication")
                                                           : QStringLiteral("Signature");

    certInfoLabel->setText(tr("%1: %2\nIssuer: %3\nValid: from %4 to %5")
                               .arg(certType, certInfo.subject, certInfo.issuer,
                                    certInfo.effectiveDate, certInfo.expiryDate));
    originLabel->setText(fromPunycode(origin));
    icon->setPixmap(certInfo.icon);

    if (pinInfo.pinRetriesCount.first == 0) {
        displayFatalError(descriptionLabel, tr("PIN is blocked, cannot proceed"));
    } else if (certStatus != CertificateStatus::VALID) {
        QString certStatusStr;
        switch (certStatus) {
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
        setupPinInputValidator(pinInfo.pinMinMaxLength);
        if (pinInfo.pinRetriesCount.first != pinInfo.pinRetriesCount.second) {
            pinErrorLabelOnPage()->setText(
                tr("%n retries left", nullptr, int(pinInfo.pinRetriesCount.first)));
        }
    }
}

void WebEidDialog::onDocumentHashReady(const QString& docHash)
{
    ui->docHashFingerprintLabel->setText(docHash);

    const auto toggleHashFingerprint = [this](bool showFingerprint) {
        const auto hash = ui->docHashFingerprintLabel->text();

        // One docHashFingerprintLabel line fits 5 quadruplets of 3 characters each.
        const auto docHashLinesHeight = lineHeight * (hash.length() / (5 * 4 * 3));

        if (showFingerprint) {
            ui->docHashFingerprintLabel->show();
            ui->docHashFingerprintToggleButton->setText(tr("Hide document fingerprint"));
            ui->docHashFingerprintToggleButton->setArrowType(Qt::ArrowType::DownArrow);

            // Resize the dialog to accommodate the hash.
            resize(width(), height() + docHashLinesHeight);
        } else {
            ui->docHashFingerprintLabel->hide();
            ui->docHashFingerprintToggleButton->setText(tr("Show document fingerprint"));
            ui->docHashFingerprintToggleButton->setArrowType(Qt::ArrowType::RightArrow);

            resize(width(), height() - docHashLinesHeight);
        }
    };

    connect(ui->docHashFingerprintToggleButton, &QToolButton::toggled, this, toggleHashFingerprint);
}

void WebEidDialog::onSigningCertificateHashMismatch()
{
    auto [descriptionLabel, originLabel, certInfoLabel, icon] = certificateLabelsOnPage();
    // TODO: Better error message/explanation.
    displayFatalError(descriptionLabel,
                      tr("Certificate on card does not match the "
                         "certificate provided as argument, cannot proceed"));
}

void WebEidDialog::onRetry(const QString& error, bool rerunFromStart)
{
    // FIXME: translation and user-friendly error messages instead of raw technical errors
    const auto result =
        QMessageBox::warning(this, tr("Retry?"), "Error occurred: " + error,
                             QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No));
    if (result == QMessageBox::Yes) {
        if (readerHasPinPad.value_or(false)) {
            startPinTimeoutProgressBar();
        }
        emit retry(rerunFromStart);
    } else {
        emit reject();
    }
}

void WebEidDialog::onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                                     const quint8 retriesLeft)
{
    using Status = electronic_id::VerifyPinFailed::Status;
    auto pinErrorLabel = pinErrorLabelOnPage();

    QString message;

    // FIXME: don't allow retry in case of PIN_BLOCKED, UNKNOWN_ERROR
    switch (status) {
    case Status::RETRY_ALLOWED:
        // Windows: An incorrect PIN was presented to the smart card: 2 retries left.
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

    pinErrorLabel->setText(message);

    if (readerHasPinPad.value_or(false)) {
        // FIXME: handle rerun from start case here, discern UNKNOWN_ERRORs better.
        onRetry(message, false);
    }
}

void WebEidDialog::makeOkButtonDefaultAndconnectSignals()
{
    auto cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);

    connect(okButton, &QPushButton::clicked, this, &WebEidDialog::onOkButtonClicked);
    connect(cancelButton, &QPushButton::clicked, this, &WebEidDialog::rejected);

    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    okButton->setDefault(true);
    okButton->setAutoDefault(true);
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
        emit waitingForPinPad();

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

std::tuple<QLabel*, QLabel*, QLabel*, QLabel*> WebEidDialog::certificateLabelsOnPage()
{
    switch (currentCommand) {
    case CommandType::GET_CERTIFICATE:
        return {ui->selectCertificateDescriptionLabel, ui->selectCertificateOriginLabel,
                ui->selectCertificateCertificateInfoLabel, ui->selectCertificateIcon};
    case CommandType::AUTHENTICATE:
        return {ui->authenticateDescriptionLabel, ui->authenticateOriginLabel,
                ui->authenticateCertificateInfoLabel, ui->authenticateElectronicIdIcon};
    case CommandType::SIGN:
        return {ui->signDescriptionLabel, ui->signOriginLabel, ui->signCertificateInfoLabel,
                ui->signElectronicIdIcon};
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
    makeLabelForegroundDarkRed(descriptionLabel);
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

    if (currentCommand == CommandType::SIGN) {
        ui->docHashFingerprintToggleButton->hide();
    }
}
