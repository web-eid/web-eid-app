/*
 * Copyright (c) 2020-2022 Estonian Information System Authority
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
#include "application.hpp"
#include "punycode.hpp"

#include "ui_dialog.h"

#include <QButtonGroup>
#include <QDesktopServices>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QMutexLocker>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QStyle>
#include <QTimeLine>
#include <QUrl>
#include <application.hpp>

#ifdef Q_OS_LINUX
#include <stdio.h>
#include <unistd.h>
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
constexpr inline QLatin1String operator"" _L1(const char* str, size_t size) noexcept
{
    return QLatin1String(str, int(size));
}
#else
using namespace Qt::Literals::StringLiterals;
#endif

using namespace electronic_id;

class WebEidDialog::Private : public Ui::WebEidDialog
{
public:
    // Non-owning observer pointers.
    QRegularExpressionValidator* pinInputValidator;
    QTimeLine* pinTimeoutTimer;
    QButtonGroup* selectionGroup;
};

WebEidDialog::WebEidDialog(QWidget* parent) : WebEidUI(parent), ui(new Private)
{
    ui->setupUi(this);
    if (qApp->isDarkTheme()) {
        QFile f(QStringLiteral(":dark.qss"));
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            setStyleSheet(styleSheet() + QTextStream(&f).readAll());
            ui->selectCertificateOriginLabelIcon->setPixmap(pixmap("origin"_L1));
            ui->pinInputOriginLabelIcon->setPixmap(pixmap("origin"_L1));
            ui->cardChipIcon->setPixmap(pixmap("no-id-card"_L1));
            ui->fatalErrorIcon->setPixmap(pixmap("fatal"_L1));
            ui->aboutIcon->setPixmap(pixmap("fatal"_L1));
            ui->helpButton->setIcon(QIcon(QStringLiteral(":/images/help_dark.svg")));
        }
    }
    setWindowFlag(Qt::CustomizeWindowHint);
    setWindowFlag(Qt::WindowTitleHint);
    setWindowTitle(qApp->applicationDisplayName());
    setTrText(ui->aboutVersion, [] { return tr("Version: %1").arg(qApp->applicationVersion()); });
    QMenu* langMenu = new QMenu(ui->langButton);
    langMenu->addAction(QStringLiteral("EN"));
    langMenu->addAction(QStringLiteral("ET"));
    langMenu->addAction(QStringLiteral("FI"));
    langMenu->addAction(QStringLiteral("HR"));
    langMenu->addAction(QStringLiteral("RU"));
    connect(langMenu, &QMenu::triggered, qApp, [this](QAction* action) {
        QSettings().setValue(QStringLiteral("lang"), action->text().toLower());
        ui->langButton->setText(action->text());
        qApp->loadTranslations();
    });
#ifdef Q_OS_LINUX
    setStyleSheet(styleSheet() + QStringLiteral("#langButton {padding-right: 15px;}"));
    connect(ui->langButton, &QToolButton::clicked, this, [this, langMenu] {
        langMenu->exec(mapToGlobal(ui->langButton->geometry().bottomLeft()));
    });
#else
    ui->langButton->setMenu(langMenu);
#endif
    ui->langButton->setText(
        QSettings().value(QStringLiteral("lang"), ui->langButton->text()).toString().toUpper());

    ui->pinInput->setAttribute(Qt::WA_MacShowFocusRect, false);
    auto pinInputFont = ui->pinInput->font();
    pinInputFont.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    ui->pinInput->setFont(pinInputFont);

    ui->waitingSpinner->load(QStringLiteral(":/images/wait.svg"));

    ui->selectionGroup = new QButtonGroup(this);
    ui->fatalError->hide();
    ui->fatalHelp->hide();
    ui->selectAnotherCertificate->hide();

    connect(ui->pageStack, &QStackedWidget::currentChanged, this, &WebEidDialog::resizeHeight);
    connect(ui->selectionGroup, qOverload<QAbstractButton*>(&QButtonGroup::buttonClicked), this,
            [this] { ui->okButton->setEnabled(true); });
    connect(ui->cancelButton, &QPushButton::clicked, this, &WebEidDialog::reject);
    connect(ui->helpButton, &QPushButton::clicked, this, [this] {
        ui->helpButton->setDown(false);
#ifdef Q_OS_LINUX
        // Launching Chrome in Linux causes the message "Opening in existing browser session." to be
        // printed to stdout, which ruins the browser-app communication channel. Redirect stdout to
        // pipe before launching the browser and restore it after to avoid this.
        int unusedPipe[2];
        int pipeFailed = pipe(unusedPipe);
        int savedStdout;
        if (!pipeFailed) {
            savedStdout = dup(1); // Save the original stdout.
            dup2(unusedPipe[1], 1); // Redirect stdout to pipe.
        }
#endif
        QDesktopServices::openUrl(
            tr("https://www.id.ee/en/article/how-to-check-that-your-id-card-reader-is-working/"));
#ifdef Q_OS_LINUX
        if (!pipeFailed) {
            fflush(stdout);
            dup2(savedStdout, 1); // Restore the original stdout.
            ::close(unusedPipe[1]);
            ::close(unusedPipe[0]);
        }
#endif
    });

    // Hide PIN-related widgets by default.
    ui->pinErrorLabel->hide();
    ui->pinInput->hide();
    ui->pinEntryTimeoutProgressBar->hide();
    ui->pinTimeRemaining->hide();

    ui->pinInputValidator = new QRegularExpressionValidator(ui->pinInput);
    ui->pinInput->setValidator(ui->pinInputValidator);
    connect(ui->pinInput, &QLineEdit::textChanged, ui->okButton, [this] {
        showPinInputWarning(false);
        ui->okButton->setEnabled(ui->pinInput->hasAcceptableInput());
    });
#ifdef Q_OS_MAC
    // Fix pressing Enter in PIN input field in macOS.
    connect(ui->pinInput, &QLineEdit::returnPressed, ui->okButton, &QPushButton::click);
#endif

    ui->pinEntryTimeoutProgressBar->setMaximum(PinInfo::PIN_PAD_PIN_ENTRY_TIMEOUT);
    ui->pinTimeoutTimer = new QTimeLine(ui->pinEntryTimeoutProgressBar->maximum() * 1000,
                                        ui->pinEntryTimeoutProgressBar);
    ui->pinTimeoutTimer->setEasingCurve(QEasingCurve::Linear);
    ui->pinTimeoutTimer->setFrameRange(ui->pinEntryTimeoutProgressBar->minimum(),
                                       ui->pinEntryTimeoutProgressBar->maximum());
    connect(ui->pinTimeoutTimer, &QTimeLine::frameChanged, ui->pinEntryTimeoutProgressBar,
            &QProgressBar::setValue);
    connect(ui->pinTimeoutTimer, &QTimeLine::frameChanged, ui->pinTimeRemaining, [this](int value) {
        ui->pinTimeRemaining->setText(
            tr("Time remaining: <b>%1</b>").arg(ui->pinEntryTimeoutProgressBar->maximum() - value));
    });

    connect(this, &WebEidDialog::languageChange, this,
            [this] { ui->pinInputCertificateInfo->languageChange(); });
}

WebEidDialog::~WebEidDialog()
{
    delete ui;
}

void WebEidDialog::showAboutPage()
{
    WebEidDialog* d = new WebEidDialog();
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->ui->helpButton->hide();
    d->ui->aboutAlert->hide();
    auto app = static_cast<Application*>(QCoreApplication::instance());
    if (app->isSafariExtensionContainingApp()) {
        d->setupOK([app] { app->showSafariSettings(); },
                   [] { return tr("Open Safari settings..."); }, true);
        connect(app, &Application::safariExtensionEnabled, d, [d](bool value) {
            d->ui->aboutAlert->setHidden(value);
            d->resizeHeight();
        });
        app->requestSafariExtensionState();
    } else {
        d->ui->okButton->hide();
    }
    d->ui->pageStack->setCurrentIndex(int(Page::ABOUT));
    d->resizeHeight();
    d->open();
    connect(d, &WebEidDialog::finished, qApp, &QApplication::quit);
}

void WebEidDialog::showFatalErrorPage()
{
    WebEidDialog* d = new WebEidDialog();
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->setTrText(d->ui->messagePageTitleLabel, [] { return tr("Operation failed"); });
    d->ui->fatalError->show();
    d->ui->fatalHelp->show();
    d->ui->connectCardLabel->hide();
    d->ui->cardChipIcon->hide();
    d->ui->helpButton->show();
    d->ui->cancelButton->show();
    d->ui->okButton->hide();
    d->ui->pageStack->setCurrentIndex(int(Page::ALERT));
    d->resizeHeight();
    d->exec();
}

void WebEidDialog::showWaitingForCardPage(const CommandType commandType)
{
    currentCommand = commandType;

    // Don't show OK button while waiting for card operation or connect card.
    ui->okButton->hide();

    ui->pageStack->setCurrentIndex(int(Page::WAITING));
}

QString WebEidDialog::getPin()
{
    // getPin() is called from background threads and must be thread-safe.
    // QString uses QAtomicPointer internally and is thread-safe.
    return pin;
}

void WebEidDialog::onSmartCardStatusUpdate(const RetriableError status)
{
    currentCommand = CommandType::INSERT_CARD;

    setTrText(ui->connectCardLabel,
              [this, status] { return std::get<0>(retriableErrorToTextTitleAndIcon(status)); });
    setTrText(ui->messagePageTitleLabel,
              [this, status] { return std::get<1>(retriableErrorToTextTitleAndIcon(status)); });
    ui->cardChipIcon->setPixmap(std::get<2>(retriableErrorToTextTitleAndIcon(status)));

    // In case the insert card page is not shown, switch back to it.
    ui->helpButton->show();
    ui->cancelButton->show();
    ui->okButton->hide();
    ui->pageStack->setCurrentIndex(int(Page::ALERT));
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
    ui->selectCertificateOriginLabel->setText(fromPunycode(origin));
    setupCertificateAndPinInfo(certificateAndPinInfos);

    switch (currentCommand) {
    case CommandType::GET_SIGNING_CERTIFICATE:
        setupOK([this] {
            if (CertificateButton* button =
                    qobject_cast<CertificateButton*>(ui->selectionGroup->checkedButton())) {

                emit accepted(button->certificateInfo());
            } else {
                emit failure(QStringLiteral("CertificateButton not found"));
            }
        });
        ui->pageStack->setCurrentIndex(int(Page::SELECT_CERTIFICATE));
        break;
    case CommandType::AUTHENTICATE:
        ui->selectAnotherCertificate->disconnect();
        ui->selectAnotherCertificate->setVisible(certificateAndPinInfos.size() > 1);
        connect(ui->selectAnotherCertificate, &QPushButton::clicked, this,
                [this, origin, certificateAndPinInfos] {
                    ui->pinInput->clear();
                    onMultipleCertificatesReady(origin, certificateAndPinInfos);
                });
        setupOK([this, origin, certificateAndPinInfos] {
            // Authenticate continues with the selected certificate to onSingleCertificateReady().
            if (CertificateButton* button =
                    qobject_cast<CertificateButton*>(ui->selectionGroup->checkedButton())) {
                onSingleCertificateReady(origin, button->certificateInfo());
            } else {
                emit failure(QStringLiteral("CertificateButton not found"));
            }
        });
        ui->pageStack->setCurrentIndex(int(Page::SELECT_CERTIFICATE));
        break;
    default:
        emit failure(
            QStringLiteral("Command %1 not allowed here").arg(std::string(currentCommand).c_str()));
    }
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
    ui->selectCertificateOriginLabel->setText(fromPunycode(origin));
    ui->pinInputOriginLabel->setText(ui->selectCertificateOriginLabel->text());
    const bool useExternalPinDialog = certAndPin.cardInfo->eid().providesExternalPinDialog();

    switch (currentCommand) {
    case CommandType::GET_SIGNING_CERTIFICATE:
        setupCertificateAndPinInfo({certAndPin});
        setupOK([this, certAndPin] { emit accepted(certAndPin); });
        ui->selectionGroup->buttons().at(0)->click();
        ui->pageStack->setCurrentIndex(int(Page::SELECT_CERTIFICATE));
        return;
    case CommandType::AUTHENTICATE:
        ui->pinInputCertificateInfo->setCertificateInfo(certAndPin);
        setTrText(ui->pinInputPageTitleLabel, [] { return tr("Authenticate"); });
        setTrText(ui->pinInputDescriptionLabel, [] {
            return tr("By authenticating, I agree to the transfer of my name and personal "
                      "identification code to the service provider.");
        });
        setTrText(ui->pinTitleLabel, [useExternalPinDialog] {
            return useExternalPinDialog
                ? tr("Please enter PIN for authentication in the PIN dialog window that opens.")
                : tr("Enter PIN1 for authentication");
        });
        break;
    case CommandType::SIGN:
        ui->pinInputCertificateInfo->setCertificateInfo(certAndPin);
        setTrText(ui->pinInputPageTitleLabel, [] { return tr("Signing"); });
        setTrText(ui->pinInputDescriptionLabel, [] {
            return tr("By signing, I agree to the transfer of my name and personal identification "
                      "code to the service provider.");
        });
        setTrText(ui->pinTitleLabel, [useExternalPinDialog] {
            return useExternalPinDialog
                ? tr("Please enter PIN for signing in the PIN dialog window that opens.")
                : tr("Enter PIN2 for signing");
        });
        break;
    default:
        emit failure(QStringLiteral("Only SELECT_CERTIFICATE, AUTHENTICATE or SIGN allowed"));
        return;
    }

    if (certAndPin.pinInfo.pinIsBlocked) {
        displayPinBlockedError();
    } else if (certAndPin.certInfo.isExpired || certAndPin.certInfo.notEffective) {
        ui->pinTitleLabel->hide();
    } else if (useExternalPinDialog) {
        connectOkToCachePinAndEmitSelectedCertificate(certAndPin);
        ui->pinInput->setText("unused"); // Dummy value as empty PIN is not allowed.
    } else if (certAndPin.pinInfo.readerHasPinPad) {
        setupPinPadProgressBarAndEmitWait(certAndPin);
    } else {
        setupPinInput(certAndPin);
    }

    ui->pageStack->setCurrentIndex(int(Page::PIN_INPUT));
}

void WebEidDialog::onRetry(const RetriableError error)
{
    onRetryImpl([this, error] { return std::get<0>(retriableErrorToTextTitleAndIcon(error)); });
}

void WebEidDialog::onSigningCertificateMismatch()
{
    onRetryImpl([] {
        return tr("The certificate of the ID card in the reader does not match the originally "
                  "submitted certificate. Please insert the original ID card.");
    });
}

void WebEidDialog::onVerifyPinFailed(const VerifyPinFailed::Status status, const qint8 retriesLeft)
{
    using Status = VerifyPinFailed::Status;

    std::function<QString()> message;

    // FIXME: don't allow retry in case of UNKNOWN_ERROR
    switch (status) {
    case Status::RETRY_ALLOWED:
        message = [retriesLeft] {
            return retriesLeft == -1 ? tr("Incorrect PIN.")
                                     : tr("Incorrect PIN, %n attempts left.", nullptr, retriesLeft);
        };
        showPinInputWarning(true);
        break;
    case Status::PIN_BLOCKED:
        displayPinBlockedError();
        resizeHeight();
        return;
    case Status::INVALID_PIN_LENGTH:
        message = [] { return tr("Invalid PIN length"); };
        break;
    case Status::PIN_ENTRY_TIMEOUT:
        message = [] { return tr("PinPad timed out waiting for customer interaction."); };
        break;
    case Status::PIN_ENTRY_CANCEL:
        message = [] { return tr("PIN entry cancelled."); };
        break;
    case Status::PIN_ENTRY_DISABLED:
    case Status::UNKNOWN_ERROR:
        message = [] { return tr("Technical error"); };
        break;
    }

    ui->pinErrorLabel->setVisible(bool(message));
    setTrText(ui->pinErrorLabel, message);

    if (ui->pinEntryTimeoutProgressBar->isVisible()) {
        onRetryImpl(message);
    } else {
        ui->pinInput->show();
        ui->pinInput->setFocus();
        ui->pinTitleLabel->show();
        ui->okButton->setDisabled(true);
        ui->cancelButton->setEnabled(true);
        resizeHeight();
    }
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
        emit languageChange();
        resizeHeight();
    }
    return WebEidUI::event(event);
}

void WebEidDialog::onRetryImpl(const std::function<QString()>& text)
{
    setTrText(ui->connectCardLabel, text);
    setTrText(ui->messagePageTitleLabel, [] { return tr("Operation failed"); });
    ui->cardChipIcon->setPixmap(pixmap("no-id-card"_L1));
    setupOK([this] { emit retry(); }, [] { return tr("Try again"); }, true);
    ui->pageStack->setCurrentIndex(int(Page::ALERT));
}

void WebEidDialog::setTrText(QWidget* label, const std::function<QString()>& text)
{
    disconnect(this, &WebEidDialog::languageChange, label, nullptr);
    label->setProperty("text", text());
    connect(this, &WebEidDialog::languageChange, label,
            [label, text] { label->setProperty("text", text()); });
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
        ui->pinInput->clear();

        emit accepted(certAndPin);
    });
}

void WebEidDialog::setupCertificateAndPinInfo(
    const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos)
{
    qDeleteAll(ui->selectCertificatePage->findChildren<CertificateButton*>());
    for (const CardCertificateAndPinInfo& certAndPin : cardCertAndPinInfos) {
        QWidget* previous = ui->selectCertificateOriginLabel;
        if (!ui->selectionGroup->buttons().isEmpty()) {
            previous = ui->selectionGroup->buttons().last();
        }
        CertificateButton* button = new CertificateButton(certAndPin, ui->selectCertificatePage);
        ui->selectCertificateInfo->addWidget(button);
        ui->selectionGroup->addButton(button);
        connect(this, &WebEidDialog::languageChange, button,
                [button] { button->languageChange(); });
        setTabOrder(previous, button);
    }
}

void WebEidDialog::setupPinPrompt(const PinInfo& pinInfo)
{
    ui->okButton->setHidden(pinInfo.readerHasPinPad);
    ui->cancelButton->setHidden(pinInfo.readerHasPinPad);
    ui->pinInput->setHidden(pinInfo.readerHasPinPad);
    ui->pinTimeRemaining->setVisible(pinInfo.readerHasPinPad);
    ui->pinEntryTimeoutProgressBar->setVisible(pinInfo.readerHasPinPad);
    bool showPinError = pinInfo.pinRetriesCount.second != -1
        && pinInfo.pinRetriesCount.first != pinInfo.pinRetriesCount.second;
    ui->pinErrorLabel->setVisible(showPinError);
    showPinInputWarning(showPinError);
    if (showPinError) {
        setTrText(ui->pinErrorLabel, [pinInfo] {
            return tr("The PIN has been entered incorrectly at least once. %n attempts left.",
                      nullptr, int(pinInfo.pinRetriesCount.first));
        });
    }
}

void WebEidDialog::setupPinPadProgressBarAndEmitWait(const CardCertificateAndPinInfo& certAndPin)
{
    setupPinPrompt(certAndPin.pinInfo);
    hide();
    setWindowFlag(Qt::WindowCloseButtonHint, false);
    show();

    ui->selectAnotherCertificate->hide();
    ui->pinTimeRemaining->setText(
        tr("Time remaining: <b>%1</b>").arg(ui->pinEntryTimeoutProgressBar->maximum()));
    setTrText(ui->pinTitleLabel, [this] {
        return tr("Please enter %1 in PinPad reader")
            .arg(currentCommand == CommandType::AUTHENTICATE ? tr("PIN1 for authentication")
                                                             : tr("PIN2 for signing"));
    });

    ui->pinEntryTimeoutProgressBar->reset();
    // To be strictly correct, the timeout timer should be started after the handler thread
    // has triggered the PIN pad internal timeout timer. However, that would involve extra
    // complexity in signal-slot setup that would bring little value as the difference between
    // timers is undetectable to the user, so we simply start the timer here, slightly earlier
    // than the PIN pad timer.
    ui->pinTimeoutTimer->start();

    emit waitingForPinPad(certAndPin);
}

void WebEidDialog::setupPinInput(const CardCertificateAndPinInfo& certAndPin)
{
    setupPinPrompt(certAndPin.pinInfo);
    // The allowed character ranges are from the SafeNet eToken guide:
    // 1. English uppercase letters (ASCII 0x41...0x5A).
    // 2. English lowercase letters (ASCII 0x61...0x7A).
    // 3. Numeric (ASCII 0x30...0x39).
    // 4. Special characters
    //    (ASCII 0x20...0x2F, space../ + 0x3A...0x40, :..@ + 0x5B...0x60, [..` + 0x7B...0x7F, {..~).
    // 5. We additionally allow uppercase and lowercase Unicode letters.
    const auto regexpWithOrWithoutLetters =
        certAndPin.cardInfo->eid().allowsUsingLettersAndSpecialCharactersInPin()
        ? QStringLiteral("[0-9 -/:-@[-`{-~\\p{L}]{%1,%2}")
        : QStringLiteral("[0-9]{%1,%2}");
    const auto numericMinMaxRegexp =
        QRegularExpression(regexpWithOrWithoutLetters.arg(certAndPin.pinInfo.pinMinMaxLength.first)
                               .arg(certAndPin.pinInfo.pinMinMaxLength.second));
    ui->pinInputValidator->setRegularExpression(numericMinMaxRegexp);
    ui->pinInput->setMaxLength(int(certAndPin.pinInfo.pinMinMaxLength.second));
    ui->pinInput->setFocus();
    connectOkToCachePinAndEmitSelectedCertificate(certAndPin);
}

void WebEidDialog::setupOK(const std::function<void()>& func, const std::function<QString()>& text,
                           bool enabled)
{
    ui->okButton->disconnect();
    connect(ui->okButton, &QPushButton::clicked, this, func);
    ui->okButton->show();
    ui->okButton->setEnabled(enabled);
    setTrText(
        ui->okButton, text ? text : [] { return tr("Confirm"); });
    ui->cancelButton->show();
    ui->cancelButton->setEnabled(true);
    ui->helpButton->hide();
}

void WebEidDialog::displayPinBlockedError()
{
    ui->pinTitleLabel->hide();
    ui->pinInput->hide();
    ui->pinTimeoutTimer->stop();
    ui->pinTimeRemaining->hide();
    ui->pinEntryTimeoutProgressBar->hide();
    setTrText(ui->pinErrorLabel, [] { return tr("PIN is locked. Unblock and try again."); });
    ui->pinErrorLabel->show();
    ui->okButton->hide();
    ui->cancelButton->setEnabled(true);
    ui->cancelButton->show();
    ui->helpButton->show();
}

void WebEidDialog::showPinInputWarning(bool show)
{
    style()->unpolish(ui->pinInput);
    ui->pinInput->setProperty("warning", show);
    style()->polish(ui->pinInput);
}

void WebEidDialog::resizeHeight()
{
    ui->pageStack->setFixedHeight(ui->pageStack->currentWidget()->sizeHint().height());
    adjustSize();
}

QPixmap WebEidDialog::pixmap(QLatin1String name) const
{
    return {QStringLiteral(":/images/%1%2.svg")
                .arg(name, qApp->isDarkTheme() ? "_dark"_L1 : QLatin1String())};
}

std::tuple<QString, QString, QPixmap>
WebEidDialog::retriableErrorToTextTitleAndIcon(const RetriableError error)
{
    switch (error) {
    case RetriableError::SMART_CARD_SERVICE_IS_NOT_RUNNING:
        return {tr("The smart card service required to use the ID-card is not running. Please "
                   "start the smart card service and try again."),
                tr("Launch the Smart Card service"), pixmap("cardreader"_L1)};
    case RetriableError::NO_SMART_CARD_READERS_FOUND:
        return {tr("<b>Card reader not connected.</b> Please connect the card reader to "
                   "the computer."),
                tr("Connect the card reader"), pixmap("cardreader"_L1)};

    case RetriableError::NO_SMART_CARDS_FOUND:
    case RetriableError::PKCS11_TOKEN_NOT_PRESENT:
        return {tr("<b>ID-card not found.</b> Please insert the ID-card into the reader."),
                tr("Insert the ID-card"), pixmap("no-id-card"_L1)};
    case RetriableError::SMART_CARD_WAS_REMOVED:
    case RetriableError::PKCS11_TOKEN_REMOVED:
        return {tr("The ID-card was removed from the reader. Please insert the ID-card into the "
                   "reader."),
                tr("Insert the ID-card"), pixmap("no-id-card"_L1)};

    case RetriableError::SMART_CARD_TRANSACTION_FAILED:
        return {tr("Operation failed. Make sure that the ID-card and the card reader are connected "
                   "correctly."),
                tr("Check the ID-card and the reader connection"), pixmap("no-id-card"_L1)};
    case RetriableError::FAILED_TO_COMMUNICATE_WITH_CARD_OR_READER:
        return {tr("Connection to the ID-card or reader failed. Make sure that the ID-card and the "
                   "card reader are connected correctly."),
                tr("Check the ID-card and the reader connection"), pixmap("no-id-card"_L1)};

    case RetriableError::SMART_CARD_CHANGE_REQUIRED:
        return {tr("The desired operation cannot be performed with the inserted ID-card. Make sure "
                   "that the ID-card is supported by the Web eID application."),
                tr("Operation not supported"), pixmap("no-id-card"_L1)};

    case RetriableError::SMART_CARD_COMMAND_ERROR:
        return {tr("Error communicating with the card."), tr("Operation failed"),
                pixmap("no-id-card"_L1)};
        // TODO: what action should the user take? Should this be fatal?
    case RetriableError::PKCS11_ERROR:
        return {tr("Card driver error. Please try again."), tr("Card driver error"),
                pixmap("no-id-card"_L1)};
        // TODO: what action should the user take? Should this be fatal?
    case RetriableError::SCARD_ERROR:
        return {tr("An error occurred in the Smart Card service required to use the ID-card. Make "
                   "sure that the ID-card and the card reader are connected correctly or relaunch "
                   "the Smart Card service."),
                tr("Operation failed"), pixmap("no-id-card"_L1)};

    case RetriableError::UNSUPPORTED_CARD:
        return {tr("The card in the reader is not supported. Make sure that the entered ID-card is "
                   "supported by the Web eID application."),
                tr("Operation not supported"), pixmap("no-id-card"_L1)};

    case RetriableError::NO_VALID_CERTIFICATE_AVAILABLE:
        return {tr("The inserted ID-card does not contain a certificate for the requested "
                   "operation. Please insert an ID-card that supports the requested operation."),
                tr("Operation not supported"), pixmap("no-id-card"_L1)};

    case RetriableError::PIN_VERIFY_DISABLED:
        return {
            tr("Operation failed. Make sure that the driver of the corresponding card reader is "
               "used. Read more <a "
               "href=\"https://www.id.ee/en/article/using-pinpad-card-reader-drivers/\">here</"
               "a>."),
            tr("Card driver error"), QStringLiteral(":/images/cardreader.svg")};

    case RetriableError::UNKNOWN_ERROR:
        return {tr("Unknown error"), tr("Unknown error"), pixmap("no-id-card"_L1)};
    }
    return {tr("Unknown error"), tr("Unknown error"), pixmap("no-id-card"_L1)};
}
