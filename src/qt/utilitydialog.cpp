// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2025-2026 The Soteria Core developer

#if defined(HAVE_CONFIG_H)
#include "config/soteria-config.h"
#endif

#include "utilitydialog.h"

#include "ui_helpmessagedialog.h"
#include "ui_paperwalletdialog.h"

#include "soteriaunits.h"

#ifdef ENABLE_WALLET
#include "guiconstants.h"
#include "walletmodel.h"
#include "sendcoinsdialog.h"
#include "sendcoinsentry.h"
#include "../wallet/coincontrol.h"
#include "coincontroldialog.h"
#endif

#include "optionsmodel.h"

#include "soteriagui.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "intro.h"
#include "paymentrequestplus.h"
#include "guiutil.h"
#include <string>
#include "clientversion.h"
#include "init.h"
#include <util/system.h>
#include "net.h"

#include <stdio.h>

#include <boost/foreach.hpp>

#include <QCloseEvent>
#include <QFont>
#include <QLabel>
#include <QRegExp>
#include <QTextTable>
#include <QTextCursor>
#include <QVBoxLayout>
#include <QInputDialog>

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

#if QT_VERSION < 0x050000
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#else
// Use QT5's new modular classes
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#endif
#include <QPainter>
#include <QPainterPath>
#include "walletmodel.h"

/** "Help message" or "About" dialog box */
HelpMessageDialog::HelpMessageDialog(QWidget *parent, bool about) :
    QDialog(parent),
    ui(new Ui::HelpMessageDialog)
{
    ui->setupUi(this);

    QString version = tr(PACKAGE_NAME) + " " + tr("version") + " " + QString::fromStdString(FormatFullVersion());
    /* On x86 add a bit specifier to the version so that users can distinguish between
     * 32 and 64 bit builds. On other architectures, 32/64 bit may be more ambiguous.
     */
#if defined(__x86_64__)
    version += " " + tr("(%1-bit)").arg(64);
#elif defined(__i386__ )
    version += " " + tr("(%1-bit)").arg(32);
#endif

    if (about)
    {
        setWindowTitle(tr("About %1").arg(tr(PACKAGE_NAME)));

        /// HTML-format the license message from the core
        QString licenseInfo = QString::fromStdString(LicenseInfo());
        QString licenseInfoHTML = licenseInfo;
        // Make URLs clickable
        QRegExp uri("<(.*)>", Qt::CaseSensitive, QRegExp::RegExp2);
        uri.setMinimal(true); // use non-greedy matching
        licenseInfoHTML.replace(uri, "<a href=\"\\1\">\\1</a>");
        // Replace newlines with HTML breaks
        licenseInfoHTML.replace("\n", "<br>");

        ui->aboutMessage->setTextFormat(Qt::RichText);
        ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        text = version + "\n" + licenseInfo;
        ui->aboutMessage->setText(version + "<br><br>" + licenseInfoHTML);
        ui->aboutMessage->setWordWrap(true);
        ui->helpMessage->setVisible(false);
    } else {
        setWindowTitle(tr("Command-line options"));
        QString header = tr("Usage:") + "\n" +
            "  soteria-qt [" + tr("command-line options") + "]                     " + "\n";
        QTextCursor cursor(ui->helpMessage->document());
        cursor.insertText(version);
        cursor.insertBlock();
        cursor.insertText(header);
        cursor.insertBlock();

        std::string strUsage = HelpMessage(HMM_SOTERIA_QT);
        const bool showDebug = gArgs.GetBoolArg("-help-debug", false);
        strUsage += HelpMessageGroup(tr("UI Options:").toStdString());
        if (showDebug) {
            strUsage += HelpMessageOpt("-allowselfsignedrootcertificates", strprintf("Allow self signed root certificates (default: %u)", DEFAULT_SELFSIGNED_ROOTCERTS));
        }
        strUsage += HelpMessageOpt("-choosedatadir", strprintf(tr("Choose data directory on startup (default: %u)").toStdString(), DEFAULT_CHOOSE_DATADIR));
        strUsage += HelpMessageOpt("-lang=<lang>", tr("Set language, for example \"de_DE\" (default: system locale)").toStdString());
        strUsage += HelpMessageOpt("-min", tr("Start minimized").toStdString());
        strUsage += HelpMessageOpt("-rootcertificates=<file>", tr("Set SSL root certificates for payment request (default: -system-)").toStdString());
        strUsage += HelpMessageOpt("-splash", strprintf(tr("Show splash screen on startup (default: %u)").toStdString(), DEFAULT_SPLASHSCREEN));
        strUsage += HelpMessageOpt("-resetguisettings", tr("Reset all settings changed in the GUI").toStdString());
        if (showDebug) {
            strUsage += HelpMessageOpt("-uiplatform", strprintf("Select platform to customize UI for (one of windows, macosx, other; default: %s)", SoteriaGUI::DEFAULT_UIPLATFORM));
        }
        QString coreOptions = QString::fromStdString(strUsage);
        text = version + "\n" + header + "\n" + coreOptions;

        QTextTableFormat tf;
        tf.setBorderStyle(QTextFrameFormat::BorderStyle_None);
        tf.setCellPadding(2);
        QVector<QTextLength> widths;
        widths << QTextLength(QTextLength::PercentageLength, 35);
        widths << QTextLength(QTextLength::PercentageLength, 65);
        tf.setColumnWidthConstraints(widths);

        QTextCharFormat bold;
        bold.setFontWeight(QFont::Bold);

        for (const QString &line : coreOptions.split("\n")) {
            if (line.startsWith("  -"))
            {
                cursor.currentTable()->appendRows(1);
                cursor.movePosition(QTextCursor::PreviousCell);
                cursor.movePosition(QTextCursor::NextRow);
                cursor.insertText(line.trimmed());
                cursor.movePosition(QTextCursor::NextCell);
            } else if (line.startsWith("   ")) {
                cursor.insertText(line.trimmed()+' ');
            } else if (line.size() > 0) {
                //Title of a group
                if (cursor.currentTable())
                    cursor.currentTable()->appendRows(1);
                cursor.movePosition(QTextCursor::Down);
                cursor.insertText(line.trimmed(), bold);
                cursor.insertTable(1, 2, tf);
            }
        }

        ui->helpMessage->moveCursor(QTextCursor::Start);
        ui->scrollArea->setVisible(false);
        ui->aboutLogo->setVisible(false);
    }
}

HelpMessageDialog::~HelpMessageDialog()
{
    delete ui;
}

void HelpMessageDialog::printToConsole()
{
    // On other operating systems, the expected action is to print the message to the console.
    fprintf(stdout, "%s\n", qPrintable(text));
}

void HelpMessageDialog::showOrPrint()
{
#if defined(WIN32)
    // On Windows, show a message box, as there is no stderr/stdout in windowed applications
    exec();
#else
    // On other operating systems, print help text to console
    printToConsole();
#endif
}

void HelpMessageDialog::on_okButton_accepted()
{
    close();
}

/** "PaperWallet" dialog box */
PaperWalletDialog::PaperWalletDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PaperWalletDialog)
{
    ui->setupUi(this);

    ui->buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);

    // Begin with a small bold monospace font for the textual version of the key and address.
    QFont font("Monospace");
    font.setBold(true);
    font.setStyleHint(QFont::TypeWriter);
    font.setPixelSize(1);
    ui->addressText->setFont(font);
    ui->addressText->setStyleSheet("{background-color:transparent;}");
    ui->privateKeyText->setFont(font);
    ui->privateKeyText->setStyleSheet("{background-color:transparent;}");
    ui->addressText->setAlignment(Qt::AlignJustify);
    ui->privateKeyText->setAlignment(Qt::AlignJustify);

    QMessageBox::critical(this, "Warning", tr("It is recommended to disconnect from the internet before printing paper wallets. Although paper wallets are generated locally, a compromised system may still expose sensitive information through screen capture, remote-access malware, clipboard history, or other monitoring tools. For improved security, use a locally connected printer rather than a network printer, since network traffic may be intercepted and some printers retain copies of printed documents. Additional risks include printer memory, print spooler storage, shoulder surfing, and accidental disclosure during the generation process. Proceed with caution and limit the amount of value stored on each address."), QMessageBox::Ok, QMessageBox::Ok);
}

void PaperWalletDialog::setModel(WalletModel *model)
{
    RandAddSeed();
    this->model = model;
    this->on_getNewAddress_clicked();
}

PaperWalletDialog::~PaperWalletDialog()
{
    delete ui;
}

void PaperWalletDialog::on_getNewAddress_clicked()
{
    // Create a new private key
    CKey privKey;
    privKey.MakeNewKey(true);

    // Derive the public key
    CPubKey pubkey = privKey.GetPubKey();

    // Derive the public key hash
    CSoteriaAddress pubkeyhash;
    pubkeyhash.Set(pubkey.GetID());

    // Create String versions of each
    std::string myPrivKey = CSoteriaSecret(privKey).ToString();
    std::string myPubKey = HexStr(pubkey.begin(), pubkey.end());
    std::string myAddress = pubkeyhash.ToString();


#ifdef USE_QRCODE
    // Generate the address QR code
    QRcode *code = QRcode_encodeString(myAddress.c_str(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!code) {
        ui->addressQRCode->setText(tr("Warning: Error encoding Address into QR Code."));
        return;
    }
    QImage myImage = QImage(code->width, code->width, QImage::Format_ARGB32);
    myImage.fill(QColor(0, 0, 0, 0));
    unsigned char* p = code->data;
    for (int y = 0; y < code->width; y++) {
        for (int x = 0; x < code->width; x++) {
            myImage.setPixel(x, y, ((*p & 1) ? 0xff000000 : 0x0));
            p++;
        }
    }
    QRcode_free(code);


    // Generate the private key QR code
    code = QRcode_encodeString(myPrivKey.c_str(), 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!code) {
        ui->privateKeyQRCode->setText(tr("Warning: Error encoding private key into QR Code."));
        return;
    }
    QImage myImagePriv = QImage(code->width, code->width, QImage::Format_ARGB32);
    myImagePriv.fill(QColor(0, 0, 0, 0));
    p = code->data;
    for (int y = 0; y < code->width; y++) {
        for (int x = 0; x < code->width; x++) {
            myImagePriv.setPixel(x, y, ((*p & 1) ? 0xff000000 : 0x0));
            p++;
        }
    }
    QRcode_free(code);

    // Populate the QR Codes
    ui->addressQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(ui->addressQRCode->width(), ui->addressQRCode->height()));
    ui->addressQRCode->setStyleSheet("{color: #ffffff;}");
    ui->privateKeyQRCode->setPixmap(QPixmap::fromImage(myImagePriv).scaled(ui->privateKeyQRCode->width(), ui->privateKeyQRCode->height()));
    ui->privateKeyQRCode->setStyleSheet("{color: #ffffff;}");

#endif

    // Populate the Texts
    ui->addressText->setText(myAddress.c_str());
    ui->privateKeyText->setText(tr(myPrivKey.c_str()));

    ui->publicKey->setHtml(myPubKey.c_str());

    // Update the fonts to fit the height of the wallet.
    // This should only really trigger the first time since the font size persists.
    double paperHeight = (double)ui->paperTemplate->height();
    double maxTextWidth = paperHeight * 0.99;
    double minTextWidth = paperHeight * 0.95;
    int pixelSizeStep = 1;

    int addressTextLength = ui->addressText->fontMetrics().boundingRect(ui->addressText->text()).width();
    QFont font = ui->addressText->font();
    for (int i = 0; i < PAPER_WALLET_READJUST_LIMIT; i++) {
        if (addressTextLength < minTextWidth) {
            font.setPixelSize(font.pixelSize() + pixelSizeStep);
            ui->addressText->setFont(font);
            addressTextLength = ui->addressText->fontMetrics().boundingRect(ui->addressText->text()).width();
        } else {
            break;
        }
    }
    if (addressTextLength > maxTextWidth) {
        font.setPixelSize(font.pixelSize() - pixelSizeStep);
        ui->addressText->setFont(font);
        addressTextLength = ui->addressText->fontMetrics().boundingRect(ui->addressText->text()).width();
    }

    int privateKeyTextLength = ui->privateKeyText->fontMetrics().boundingRect(ui->privateKeyText->text()).width();
    font = ui->privateKeyText->font();
    for (int i = 0; i < PAPER_WALLET_READJUST_LIMIT; i++) {
        if (privateKeyTextLength < minTextWidth) {
            font.setPixelSize(font.pixelSize() + pixelSizeStep);
            ui->privateKeyText->setFont(font);
            privateKeyTextLength = ui->privateKeyText->fontMetrics().boundingRect(ui->privateKeyText->text()).width();
        } else {
            break;
        }
    }
    if (privateKeyTextLength > maxTextWidth) {
        font.setPixelSize(font.pixelSize() - pixelSizeStep);
        ui->privateKeyText->setFont(font);
        privateKeyTextLength = ui->privateKeyText->fontMetrics().boundingRect(ui->privateKeyText->text()).width();
    }
}

void PaperWalletDialog::on_printButton_clicked()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog* qpd = new QPrintDialog(&printer, this);

    qpd->setPrintRange(QAbstractPrintDialog::AllPages);

    QList<QString> recipientPubKeyHashes;

    if (qpd->exec() != QDialog::Accepted) {
        return;
    }

    // Hardcode these values
    printer.setOrientation(QPrinter::Portrait);
    printer.setPaperSize(QPrinter::A4);
    printer.setFullPage(true);

    QPainter painter;
    if (!painter.begin(&printer)) { // failed to open file
        QMessageBox::critical(this, "Printing Error", tr("failed to open file, is it writable?"), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    int walletCount = ui->walletCount->currentIndex() + 1;
    int walletsPerPage = 4;

    int pageHeight = printer.pageRect().height() - PAPER_WALLET_PAGE_MARGIN;
    int walletHeight = ui->paperTemplate->height();
    double computedWalletHeight = 0.9 * pageHeight / walletsPerPage;
    double scale = computedWalletHeight / walletHeight;
    double walletPadding = pageHeight * 0.05 / (walletsPerPage - 1) / scale;

    QRegion walletRegion = QRegion(ui->paperTemplate->x(), ui->paperTemplate->y(), ui->paperTemplate->width(), ui->paperTemplate->height());
    painter.scale(scale, scale);

    for (int i = 0; i < walletCount; i++) {
        QPoint point = QPoint(PAPER_WALLET_PAGE_MARGIN, (PAPER_WALLET_PAGE_MARGIN / 2) + (i % walletsPerPage) * (walletHeight + walletPadding));
        this->render(&painter, point, walletRegion);
        recipientPubKeyHashes.append(ui->addressText->text());

        if (i % walletsPerPage == (walletsPerPage - 1)) {
            printer.newPage();
        }

        this->on_getNewAddress_clicked();
    }

    painter.end();

#ifdef ENABLE_WALLET
    QStringList formatted;

    WalletModelTransaction* tx;
    while (true) {
        bool ok;

        // Ask for an amount to send to each paper wallet. It might be better to try to use the SoteriaLiteAmountField, but this works fine.
        double amountInput = QInputDialog::getDouble(this, tr("Load Paper Wallets"), tr("The paper wallet printing process has begun.<br/>Please wait for the wallet to print completely and verify that everything printed correctly.<br/>Check for misalignments, ink bleeding, smears, or anything else that could make the private keys unreadable.<br/>Now, enter the number of SOTER you wish to send to each wallet:"), 0, 1, 10000, 8, &ok);

        if (!ok) {
            return;
        }


        WalletModel::UnlockContext ctx(this->model->requestUnlock());
        if (!ctx.isValid()) {
            return;
        }

        QList<SendCoinsRecipient> recipients;
        quint64 amount = (quint64)(amountInput * COIN);
        BOOST_FOREACH(const QString& dest, recipientPubKeyHashes) {
            recipients.append(SendCoinsRecipient(dest, tr("Paper wallet %1").arg(dest), amount, ""));
            formatted.append(tr("<b>%1</b> to Paper Wallet <span style='font-family: monospace;'>%2</span>").arg(QString::number(amountInput, 'f', 8), GUIUtil::HtmlEscape(dest)));
        }

        tx = new WalletModelTransaction(recipients);

        CCoinControl ctrl;
        if (this->model->getOptionsModel()->getCoinControlFeatures())
            ctrl = *CoinControlDialog::coinControl;

        WalletModel::SendCoinsReturn prepareStatus;
        prepareStatus = this->model->prepareTransaction(*tx, ctrl);

        if (prepareStatus.status == WalletModel::InvalidAddress) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The recipient address is not valid, please recheck."), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::InvalidAmount) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The amount to pay must be larger than 0"), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::AmountExceedsBalance) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The amount exceeds your balance."), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::AmountWithFeeExceedsBalance) {
            QMessageBox::critical(this, tr("Send Coins"), tr("The total exceeds your balance when the transaction fee is included"), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::DuplicateAddress) {
            QMessageBox::critical(this, tr("Send Coins"), tr("Duplicate address found, can only send to each address once per send operation."), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::TransactionCreationFailed) {
            QMessageBox::critical(this, tr("Send Coins"), tr("Transaction creation failed!"), QMessageBox::Ok, QMessageBox::Ok);
        } else if (prepareStatus.status == WalletModel::OK) {
            break;
        } else {
            delete tx;
            return;
        }
    }

    // Stolen from sendcoinsdialog.cpp
    qint64 txFee = tx->getTransactionFee();
    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if (txFee > 0) {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(SoteriaUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    qint64 totalAmount = tx->getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    BOOST_FOREACH(SoteriaUnits::Unit u, SoteriaUnits::availableUnits()) {
        if (u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(SoteriaUnits::formatWithUnit(u, totalAmount));
    }

    questionString.append(tr("Total Amount %1 (= %2)")
                              .arg(SoteriaUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
                              .arg(alternativeUnits.join(" " + tr("or") + " ")));

    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"), questionString.arg(formatted.join("<br />")), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) {
        delete tx;
        return;
    }

    WalletModel::SendCoinsReturn sendStatus = this->model->sendCoins(*tx);

    if (sendStatus.status == WalletModel::TransactionCommitFailed) {
        QMessageBox::critical(this, tr("Send Coins"), tr("The transaction was rejected. This can happen if some of your coins were already spent in another copy of your wallet, or if your wallet contains too many small outputs and needs to be consolidated before sending."), QMessageBox::Ok, QMessageBox::Ok);
    }
    delete tx;
#endif
    return;
}

/** "Shutdown" window */
ShutdownWindow::ShutdownWindow(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(new QLabel(
        tr("%1 is shutting down...").arg(tr(PACKAGE_NAME)) + "<br /><br />" +
        tr("Warning: Do not shut down the computer until this window disappears.")));
    setLayout(layout);
}

QWidget *ShutdownWindow::showShutdownWindow(SoteriaGUI *window)
{
    if (!window)
        return nullptr;

    // Show a simple window indicating shutdown status
    QWidget *shutdownWindow = new ShutdownWindow();
    shutdownWindow->setWindowTitle(window->windowTitle());

    // Center shutdown window at where main window was
    const QPoint global = window->mapToGlobal(window->rect().center());
    shutdownWindow->move(global.x() - shutdownWindow->width() / 2, global.y() - shutdownWindow->height() / 2);
    shutdownWindow->show();
    return shutdownWindow;
}



void ShutdownWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
}


