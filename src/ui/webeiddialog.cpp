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
#include "ui_dialog.h"

#include <QPushButton>
#include <QToolButton>
#include <QRegularExpressionValidator>

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

    // Hide document hash fingerprint label by default.
    ui->docHashFingerprintLabel->hide();
}

WebEidDialog::~WebEidDialog()
{
    delete ui;
}

void WebEidDialog::switchPage(const CommandType commandType)
{
    currentCommand = commandType;

    // Don't show OK button on connect smart card page.
    okButton->setHidden(commandType == CommandType::INSERT_CARD);

    ui->pageStack->setCurrentIndex(int(commandType));
}

void WebEidDialog::onReaderMonitorStatusUpdate(const electronic_id::AutoSelectFailed::Reason status)
{
    using Reason = electronic_id::AutoSelectFailed::Reason;

    switch (status) {
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
        ui->smartCardErrorLabel->setText(tr("No smart card in reader. "
                                            "Please insert a smart card into the reader."));
        break;
    case Reason::SINGLE_READER_UNSUPPORTED_CARD:
    case Reason::MULTIPLE_READERS_NO_SUPPORTED_CARD:
        ui->smartCardErrorLabel->setText(tr("Unsupported card in reader. "
                                            "Please insert Electronic ID card into the reader."));
        break;
    case Reason::MULTIPLE_SUPPORTED_CARDS:
        // TODO: Here we should display a multi-select prompt instead.
        ui->smartCardErrorLabel->setText(
            tr("Multiple Electronic ID cards inserted. Please assure that "
               "only single ID card is inserted."));
        break;
    }

    if (ui->spinnerLabel->text() == "....") {
        ui->spinnerLabel->clear();
    } else {
        ui->spinnerLabel->setText(ui->spinnerLabel->text() += '.');
    }
}

void WebEidDialog::onCertificateReady(const QString& origin, const CertificateStatus certStatus,
                                      const CertificateInfo& certInfo)
{
    auto [descriptionLabel, originLabel, certInfoLabel, icon] = certificateLabelsOnPage();

    const auto certType = certInfo.type.isAuthentication() ? QStringLiteral("Authentication")
                                                           : QStringLiteral("Signature");

    certInfoLabel->setText(tr("%1: %2\nIssuer: %3\nValid: from %4 to %5")
                               .arg(certType, certInfo.subject, certInfo.issuer,
                                    certInfo.effectiveDate, certInfo.expiryDate));
    originLabel->setText(origin);
    icon->setPixmap(certInfo.icon);

    if (certInfo.pinRetriesCount.first == 0) {
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
        setupPinInputValidator(certInfo.pinMinMaxLength);
        if (certInfo.pinRetriesCount.first != certInfo.pinRetriesCount.second) {
            pinErrorLabelOnPage()->setText(
                tr("%n retries left", nullptr, int(certInfo.pinRetriesCount.first)));
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

void WebEidDialog::onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                                     const quint8 retriesLeft)
{
    using Status = electronic_id::VerifyPinFailed::Status;
    auto pinErrorLabel = pinErrorLabelOnPage();

    switch (status) {
    case Status::RETRY_ALLOWED:
        // Windows: An incorrect PIN was presented to the smart card: 2 retries left.
        pinErrorLabel->setText(tr("Incorrect PIN, %n retries left", nullptr, retriesLeft));
        break;
    case Status::PIN_BLOCKED:
        pinErrorLabel->setText(tr("PIN blocked"));
        break;
    case Status::INVALID_PIN_LENGTH:
        pinErrorLabel->setText(tr("Wrong PIN length"));
        break;
    case Status::PIN_ENTRY_TIMEOUT:
        pinErrorLabel->setText(tr("Timeout while waiting for PIN entry"));
        break;
    case Status::PIN_ENTRY_CANCEL:
        pinErrorLabel->setText(tr("PIN entry cancelled"));
        break;
    default: // Covers Status::UNKNOWN_ERROR.
        pinErrorLabel->setText(tr("Technical error"));
    }
}

void WebEidDialog::makeOkButtonDefaultAndconnectSignals()
{
    auto cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);

    connect(okButton, &QPushButton::clicked, this, &WebEidDialog::accepted);
    connect(cancelButton, &QPushButton::clicked, this, &WebEidDialog::rejected);

    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    okButton->setDefault(true);
    okButton->setAutoDefault(true);
}

void WebEidDialog::setupPinInputValidator(const CertificateInfo::PinMinMaxLength& pinMinMaxLenght)
{
    // Do nothing in case the PIN widgets are not on the page.
    if (currentCommand != CommandType::AUTHENTICATE && currentCommand != CommandType::SIGN) {
        return;
    }

    auto pinInput = currentCommand == CommandType::AUTHENTICATE ? ui->authenticationPinInput
                                                                : ui->signingPinInput;

    okButton->setEnabled(false);

    pinInput->setMaxLength(int(pinMinMaxLenght.second));

    const auto numericMinMaxRegexp = QRegularExpression(
        QStringLiteral("[0-9]{%1,%2}").arg(pinMinMaxLenght.first).arg(pinMinMaxLenght.second));
    pinInput->setValidator(new QRegularExpressionValidator(numericMinMaxRegexp, pinInput));

    connect(pinInput, &QLineEdit::textChanged, okButton,
            [this, pinInput] { okButton->setEnabled(pinInput->hasAcceptableInput()); });

    pinInput->setFocus();
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

    using WidgetTuple = std::tuple<QWidget*, QWidget*>;

    const bool isAuthentication = currentCommand == CommandType::AUTHENTICATE;

    auto [pinTitleLabel, pinInput] = isAuthentication
        ? WidgetTuple {ui->authenticationPinTitleLabel, ui->authenticationPinInput}
        : WidgetTuple {ui->signingPinTitleLabel, ui->signingPinInput};

    pinTitleLabel->hide();
    pinInput->hide();

    if (!isAuthentication) {
        ui->docHashFingerprintToggleButton->hide();
    }
}
