/*
 * Copyright (c) 2020-2023 Estonian Information System Authority
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

#include <QActionGroup>
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

#ifdef Q_OS_LINUX
#include <stdio.h>
#include <unistd.h>
#endif

#include <unordered_map>

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
    observer_ptr<QRegularExpressionValidator> pinInputValidator;
    observer_ptr<QTimeLine> pinTimeoutTimer;
    observer_ptr<QButtonGroup> selectionGroup;
    observer_ptr<QToolButton> langButton;
};

WebEidDialog::WebEidDialog(QWidget* parent) : WebEidUI(parent), ui(new Private)
{
    // close() deletes the dialog automatically if the Qt::WA_DeleteOnClose flag is set.
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    if (Application::isDarkTheme()) {
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
    setWindowTitle(QApplication::applicationDisplayName());
    setTrText(ui->aboutVersion,
              [] { return tr("Version: %1").arg(QApplication::applicationVersion()); });

    ui->langButton = new QToolButton(this);
    ui->langButton->setObjectName("langButton");
    static const std::vector<std::pair<QString, QString>> LANG_LIST {
        {QStringLiteral("et"), QStringLiteral("Eesti")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("ru"), QStringLiteral("Русский")},
        {QStringLiteral("fi"), QStringLiteral("Suomi")},
        {QStringLiteral("hr"), QStringLiteral("Hrvatska")},
        {QStringLiteral("de"), QStringLiteral("Deutsch")},
        {QStringLiteral("fr"), QStringLiteral("Française")},
        {QStringLiteral("nl"), QStringLiteral("Nederlands")},
        {QStringLiteral("cs"), QStringLiteral("Čeština")},
        {QStringLiteral("sk"), QStringLiteral("Slovenština")}};
    ui->langButton->setText(tr("EN", "Active language"));
    if (auto i = std::find_if(
            LANG_LIST.cbegin(), LANG_LIST.cend(),
            [&](const auto& elem) { return elem.first == ui->langButton->text().toLower(); });
        i != LANG_LIST.cend()) {
        ui->langButton->setAccessibleName(i->second);
    }
    connect(ui->langButton, &QToolButton::clicked, this, [this] {
        if (auto* menu = findChild<QWidget*>(QStringLiteral("langMenu"))) {
            menu->deleteLater();
            return;
        }
        auto* menu = new QWidget(this);
        menu->setObjectName("langMenu");
        auto* layout = new QVBoxLayout(menu);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(1);
        auto* langGroup = new QButtonGroup(menu);
        langGroup->setExclusive(true);
        for (const auto& [lang, title] : LANG_LIST) {
            auto* action = new QPushButton(menu);
            action->setText(title);
            action->setProperty("lang", lang);
            action->setAutoDefault(false);
            layout->addWidget(action);
            langGroup->addButton(action);
            action->setCheckable(true);
            action->setChecked(lang == ui->langButton->text().toLower());
        }
        menu->show();
        menu->adjustSize();
        menu->move(ui->langButton->geometry().bottomRight() - QPoint(menu->width() - 1, -2));
        connect(langGroup, qOverload<QAbstractButton*>(&QButtonGroup::buttonClicked), menu,
                [this, menu](QAbstractButton* action) {
                    QSettings().setValue(QStringLiteral("lang"), action->property("lang"));
                    ui->langButton->setText(action->property("lang").toString().toUpper());
                    qApp->loadTranslations();
                    menu->deleteLater();
                });
    });

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
            [this] {
                ui->okButton->setEnabled(true);
                ui->okButton->setFocus();
            });
    connect(ui->cancelButton, &QPushButton::clicked, this, &WebEidDialog::reject);
    connect(ui->helpButton, &QPushButton::clicked, this, [this] {
        ui->helpButton->setDown(false);
#ifdef Q_OS_LINUX
        // Launching Chrome in Linux causes the message "Opening in existing browser session." to be
        // printed to stdout, which ruins the browser-app communication channel. Redirect stdout to
        // pipe before launching the browser and restore it after to avoid this.
        std::array<int, 2> unusedPipe {};
        int pipeFailed = pipe(unusedPipe.data());
        int savedStdout {};
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
            if (savedStdout >= 0) {
                dup2(savedStdout, 1); // Restore the original stdout.
                ::close(savedStdout);
            }
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
    auto* d = new WebEidDialog();
    d->ui->helpButton->hide();
    d->ui->aboutAlert->hide();
    auto* app = qApp;
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
    auto* d = new WebEidDialog();
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
              [status] { return std::get<0>(retriableErrorToTextTitleAndIcon(status)); });
    setTrText(ui->messagePageTitleLabel,
              [status] { return std::get<1>(retriableErrorToTextTitleAndIcon(status)); });
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
            if (auto* button =
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
            if (auto* button =
                    qobject_cast<CertificateButton*>(ui->selectionGroup->checkedButton())) {
                onSingleCertificateReady(origin, button->certificateInfo());
            } else {
                emit failure(QStringLiteral("CertificateButton not found"));
            }
        });
        ui->pageStack->setCurrentIndex(int(Page::SELECT_CERTIFICATE));
        break;
    default:
        emit failure(QStringLiteral("Command %1 not allowed here")
                         .arg(QString::fromStdString(currentCommand)));
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
        ui->pinInput->setText(
            QString::fromLatin1("unused")); // Dummy value as empty PIN is not allowed.
    } else if (certAndPin.pinInfo.readerHasPinPad) {
        setupPinPadProgressBarAndEmitWait(certAndPin);
    } else {
        setupPinInput(certAndPin);
    }

    ui->pageStack->setCurrentIndex(int(Page::PIN_INPUT));
}

void WebEidDialog::onRetry(const RetriableError error)
{
    onRetryImpl([error] { return std::get<0>(retriableErrorToTextTitleAndIcon(error)); });
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
        onRetryImpl(std::move(message));
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
    switch (event->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        emit languageChange();
        resizeHeight();
        break;
    case QEvent::MouseButtonRelease:
        if (auto* w = findChild<QWidget*>(QStringLiteral("langMenu"))) {
            w->deleteLater();
        }
        break;
    case QEvent::Resize:
        ui->langButton->move(width() - ui->langButton->width() - 20, 5);
        break;
    default:
        break;
    }
    return WebEidUI::event(event);
}

template <typename Text>
void WebEidDialog::onRetryImpl(Text text)
{
    setTrText(ui->connectCardLabel, std::forward<Text>(text));
    setTrText(ui->messagePageTitleLabel, [] { return tr("Operation failed"); });
    ui->cardChipIcon->setPixmap(pixmap("no-id-card"_L1));
    setupOK([this] { emit retry(); }, [] { return tr("Try again"); }, true);
    ui->pageStack->setCurrentIndex(int(Page::ALERT));
}

template <typename Text>
void WebEidDialog::setTrText(QWidget* label, Text text) const
{
    disconnect(this, &WebEidDialog::languageChange, label, nullptr);
    label->setProperty("text", text());
    connect(this, &WebEidDialog::languageChange, label,
            [label, text = std::forward<Text>(text)] { label->setProperty("text", text()); });
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

        // We use setText instead of clear, so undo/redo will be cleared as well
        ui->pinInput->setText("");
        // TODO: To be sure that no copy of PIN text remains in memory, a custom
        // widget should be implemented, that:
        // - Stores PIN in locked byte vector
        // - DOes not leak content through accessibility interface

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
        auto* button = new CertificateButton(certAndPin, ui->selectCertificatePage);
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

template <typename Func>
void WebEidDialog::setupOK(Func&& func, const std::function<QString()>& text, bool enabled)
{
    ui->okButton->disconnect();
    connect(ui->okButton, &QPushButton::clicked, this, std::forward<Func>(func));
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

QPixmap WebEidDialog::pixmap(QLatin1String name)
{
    return {QStringLiteral(":/images/%1%2.svg")
                .arg(name, Application::isDarkTheme() ? "_dark"_L1 : QLatin1String())};
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
        return {tr("Error communicating with the card. Please try again."), tr("Operation failed"),
                pixmap("no-id-card"_L1)};
    case RetriableError::PKCS11_ERROR:
        return {tr("Card driver error. Please try again."), tr("Card driver error"),
                pixmap("no-id-card"_L1)};
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
