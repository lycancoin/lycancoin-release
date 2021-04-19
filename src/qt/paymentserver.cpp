// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileOpenEvent>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStringList>
#include <QTextDocument>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QSslError>
#include <QSslSocket>
#if QT_VERSION < 0x050000
#include <QUrl>
#else
#include <QUrlQuery>
#endif

#include <cstdlib>

#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "base58.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "paymentserver.h"
#include "ui_interface.h"
#include "util.h"
#include "wallet.h"
#include "walletmodel.h"

using namespace boost;

const int BITCOIN_IPC_CONNECT_TIMEOUT = 1000; // milliseconds
const QString BITCOIN_IPC_PREFIX("lycancoin:");
const char* BITCOIN_REQUEST_MIMETYPE = "application/lycancoin-paymentrequest";
const char* BITCOIN_PAYMENTACK_MIMETYPE = "application/lycancoin-paymentack";

X509_STORE* PaymentServer::certStore = NULL;
void PaymentServer::freeCertStore()
{
    if (PaymentServer::certStore != NULL)
    {
        X509_STORE_free(PaymentServer::certStore);
        PaymentServer::certStore = NULL;
    }
}

//
// Create a name that is unique for:
//  testnet / non-testnet
//  data directory
//
static QString ipcServerName()
{
    QString name("LycancoinQt");

    // Append a simple hash of the datadir
    // Note that GetDataDir(true) returns a different path
    // for -testnet versus main net
    QString ddir(GetDataDir(true).string().c_str());
    name.append(QString::number(qHash(ddir)));

    return name;
}

//
// We store payment URLs and requests received before
// the main GUI window is up and ready to ask the user
// to send payment.

static QList<QString> savedPaymentRequests;

static void ReportInvalidCertificate(const QSslCertificate& cert)
{
    if (fDebug) {
        qDebug() << "ReportInvalidCertificate : Payment server found an invalid certificate: " << cert.subjectInfo(QSslCertificate::CommonName);
    }
}

//
// Load OpenSSL's list of root certificate authorities
//
void PaymentServer::LoadRootCAs(X509_STORE* _store)
{
    if (PaymentServer::certStore == NULL)
        atexit(PaymentServer::freeCertStore);
    else
        freeCertStore();

    // Unit tests mostly use this, to pass in fake root CAs:
    if (_store)
    {
        PaymentServer::certStore = _store;
        return;
    }

    // Normal execution, use either -rootcertificates or system certs:
    PaymentServer::certStore = X509_STORE_new();

    // Note: use "-system-" default here so that users can pass -rootcertificates=""
    // and get 'I don't like X.509 certificates, don't trust anybody' behavior:
    QString certFile = QString::fromStdString(GetArg("-rootcertificates", "-system-"));

    if (certFile.isEmpty())
        return; // Empty store

    QList<QSslCertificate> certList;

    if (certFile != "-system-")
    {
        certList = QSslCertificate::fromPath(certFile);
        // Use those certificates when fetching payment requests, too:
        QSslSocket::setDefaultCaCertificates(certList);
    }
    else
        certList = QSslSocket::systemCaCertificates ();

    int nRootCerts = 0;
    const QDateTime currentTime = QDateTime::currentDateTime();
    foreach (const QSslCertificate& cert, certList)
    {
        if (currentTime < cert.effectiveDate() || currentTime > cert.expiryDate()) {
            ReportInvalidCertificate(cert);
            continue;
        }
#if QT_VERSION >= 0x050000
        if (cert.isBlacklisted()) {
            ReportInvalidCertificate(cert);
            continue;
        }
#endif
        QByteArray certData = cert.toDer();
        const unsigned char *data = (const unsigned char *)certData.data();

        X509* x509 = d2i_X509(0, &data, certData.size());
        if (x509 && X509_STORE_add_cert(PaymentServer::certStore, x509))
        {
            // Note: X509_STORE_free will free the X509* objects when
            // the PaymentServer is destroyed
            ++nRootCerts;
        }
        else
        {
            ReportInvalidCertificate(cert);
            continue;
        }
    }
    if (fDebug)
        qDebug() << "PaymentServer::LoadRootCAs : Loaded " << nRootCerts << " root certificates";

    // Project for another day:
    // Fetch certificate revocation lists, and add them to certStore.
    // Issues to consider:
    //   performance (start a thread to fetch in background?)
    //   privacy (fetch through tor/proxy so IP address isn't revealed)
    //   would it be easier to just use a compiled-in blacklist?
    //    or use Qt's blacklist?
    //   "certificate stapling" with server-side caching is more efficient
}

//
// Sending to the server is done synchronously, at startup.
// If the server isn't already running, startup continues,
// and the items in savedPaymentRequest will be handled
// when uiReady() is called.
//
bool PaymentServer::ipcSendCommandLine(int argc, char* argv[])
{
    bool fResult = false;

    for (int i = 1; i < argc; i++)
    {
        QString arg(argv[i]);
        if (arg.startsWith("-"))
            continue;

        if (arg.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // lycancoin:
        {
            savedPaymentRequests.append(arg);

            SendCoinsRecipient r;
            if (GUIUtil::parseBitcoinURI(arg, &r))
            {
                CBitcoinAddress address(r.address.toStdString());

                SelectParams(CChainParams::MAIN);
                if (!address.IsValid())
                {
                    SelectParams(CChainParams::TESTNET);
                }
            }
        }
        else if (QFile::exists(arg)) // Filename
        {
            savedPaymentRequests.append(arg);

            PaymentRequestPlus request;
            if (readPaymentRequest(arg, request))
            {
                if (request.getDetails().network() == "main")
                    SelectParams(CChainParams::MAIN);
                else
                    SelectParams(CChainParams::TESTNET);
            }
        }
        else
        {
            qDebug() << "PaymentServer::ipcSendCommandLine : Payment request file does not exist: " << argv[i];
            // Printing to debug.log is about the best we can do here, the
            // GUI hasn't started yet so we can't pop up a message box.
        }
    }

    foreach (const QString& r, savedPaymentRequests)
    {
        QLocalSocket* socket = new QLocalSocket();
        socket->connectToServer(ipcServerName(), QIODevice::WriteOnly);
        if (!socket->waitForConnected(BITCOIN_IPC_CONNECT_TIMEOUT))
            return false;

        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << r;
        out.device()->seek(0);
        socket->write(block);
        socket->flush();

        socket->waitForBytesWritten(BITCOIN_IPC_CONNECT_TIMEOUT);
        socket->disconnectFromServer();
        delete socket;
        fResult = true;
    }
    return fResult;
}

PaymentServer::PaymentServer(QObject* parent,
                             bool startLocalServer) : QObject(parent), saveURIs(true)
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Install global event filter to catch QFileOpenEvents on the mac (sent when you click lycancoin: links)
    if (parent)
        parent->installEventFilter(this);

    QString name = ipcServerName();

    // Clean up old socket leftover from a crash:
    QLocalServer::removeServer(name);

    if (startLocalServer)
    {
        uriServer = new QLocalServer(this);

        if (!uriServer->listen(name))
            qDebug() << "PaymentServer::PaymentServer : Cannot start bitcoin: click-to-pay handler";
        else
            connect(uriServer, SIGNAL(newConnection()), this, SLOT(handleURIConnection()));
    }

    // netManager is null until uiReady() is called
    netManager = NULL;
}

PaymentServer::~PaymentServer()
{
    google::protobuf::ShutdownProtobufLibrary();
}

//
// OSX-specific way of handling lycancoin uris and
// PaymentRequest mime types
//
bool PaymentServer::eventFilter(QObject *, QEvent *event)
{
    // clicking on lycancoin: URLs creates FileOpen events on the Mac:
    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);
        if (!fileEvent->file().isEmpty())
            handleURIOrFile(fileEvent->file());
        else if (!fileEvent->url().isEmpty())
            handleURIOrFile(fileEvent->url().toString());

        return true;
    }
    return false;
}

void PaymentServer::initNetManager()
{
	 if (!optionsModel)
        return;
    if (netManager != NULL)
        delete netManager;

    // netManager is used to fetch paymentrequests given in lycancoin: URI's
    netManager = new QNetworkAccessManager(this);

    // Use proxy settings from optionsModel:
    QString proxyIP;
    quint16 proxyPort;
    if (optionsModel->getProxySettings(proxyIP, proxyPort))
    {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(proxyIP);
        proxy.setPort(proxyPort);
        netManager->setProxy(proxy);
    }

    connect(netManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(netRequestFinished(QNetworkReply*)));
    connect(netManager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
            this, SLOT(reportSslErrors(QNetworkReply*, const QList<QSslError> &)));
}

void PaymentServer::uiReady()
{
    assert(netManager != NULL); // Must call initNetManager before uiReady()

    saveURIs = false;
    foreach (const QString& s, savedPaymentRequests)
    {
        handleURIOrFile(s);
    }
    savedPaymentRequests.clear();
}

void PaymentServer::handleURIOrFile(const QString& s)
{
    if (saveURIs)
    {
        savedPaymentRequests.append(s);
        return;
    }

    if (s.startsWith(BITCOIN_IPC_PREFIX, Qt::CaseInsensitive)) // lycancoin:
    {
#if QT_VERSION >= 0x050000
        QUrlQuery url((QUrl(s)));
#else
        QUrl url(s);
#endif
        if (url.hasQueryItem("request"))
        {
            QByteArray temp; temp.append(url.queryItemValue("request"));
            QString decoded = QUrl::fromPercentEncoding(temp);
            QUrl fetchUrl(decoded, QUrl::StrictMode);

            if (fDebug)
                qDebug() << "PaymentServer::handleURIOrFile : fetchRequest(" << fetchUrl << ")";

            if (fetchUrl.isValid())
                fetchRequest(fetchUrl);
            else
                qDebug() << "PaymentServer::handleURIOrFile : Invalid url: " << fetchUrl;
            return;
        }

        SendCoinsRecipient recipient;
        if (GUIUtil::parseBitcoinURI(s, &recipient))
            emit receivedPaymentRequest(recipient);
        return;
    }

    if (QFile::exists(s))
    {
        PaymentRequestPlus request;
        QList<SendCoinsRecipient> recipients;
        if (readPaymentRequest(s, request) && processPaymentRequest(request, recipients)) {
            foreach (const SendCoinsRecipient& recipient, recipients){
                emit receivedPaymentRequest(recipient);
            }
        }
        return;
    }
}

void PaymentServer::handleURIConnection()
{
    QLocalSocket *clientConnection = uriServer->nextPendingConnection();

    while (clientConnection->bytesAvailable() < (int)sizeof(quint32))
        clientConnection->waitForReadyRead();

    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    QDataStream in(clientConnection);
    in.setVersion(QDataStream::Qt_4_0);
    if (clientConnection->bytesAvailable() < (int)sizeof(quint16)) {
        return;
    }
    QString message;
    in >> message;

    handleURIOrFile(message);
}

bool PaymentServer::readPaymentRequest(const QString& filename, PaymentRequestPlus& request)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
    {
        qDebug() << "PaymentServer::readPaymentRequest : Failed to open " << filename;
        return false;
    }

    if (f.size() > MAX_PAYMENT_REQUEST_SIZE)
    {
        qDebug() << "PaymentServer::readPaymentRequest : " << filename << " too large";
        return false;
    }

    QByteArray data = f.readAll();

    return request.parse(data);
}

bool
PaymentServer::processPaymentRequest(PaymentRequestPlus& request,
                                     QList<SendCoinsRecipient>& recipients)
{
    if (!optionsModel)
        return false;
    QList<std::pair<CScript,qint64> > sendingTos = request.getPayTo();
    qint64 totalAmount = 0;
    foreach(const PAIRTYPE(CScript, qint64)& sendingTo, sendingTos) {
        CTxOut txOut(sendingTo.second, sendingTo.first);
        if (txOut.IsDust(CTransaction::nMinRelayTxFee)) {
            QString message = QObject::tr("Requested payment amount (%1) too small")
                .arg(BitcoinUnits::formatWithUnit(optionsModel->getDisplayUnit(), sendingTo.second));

            qDebug() << "PaymentServer::processPaymentRequest : " << message;
            emit reportError(tr("Payment request error"), message, CClientUIInterface::MODAL);
            return false;
        }

        totalAmount += sendingTo.second;
    }

    recipients.append(SendCoinsRecipient());

    if (request.getMerchant(PaymentServer::certStore, recipients[0].authenticatedMerchant)) {
        recipients[0].paymentRequest = request;
        recipients[0].amount = totalAmount;
        if (fDebug)
            qDebug() << "PaymentServer::processPaymentRequest : Payment request from " << recipients[0].authenticatedMerchant;
    }
    else {
        recipients.clear();
        // Insecure payment requests may turn into more than one recipient if
        // the merchant is requesting payment to more than one address.
        for (int i = 0; i < sendingTos.size(); i++) {
            std::pair<CScript, qint64>& sendingTo = sendingTos[i];
            recipients.append(SendCoinsRecipient());
            recipients[i].amount = sendingTo.second;
            QString memo = QString::fromStdString(request.getDetails().memo());
#if QT_VERSION < 0x050000
            recipients[i].label = Qt::escape(memo);
#else
            recipients[i].label = memo.toHtmlEscaped();
#endif
            CTxDestination dest;
            if (ExtractDestination(sendingTo.first, dest)) {
                if (i == 0) // Tie request to first pay-to, we don't want multiple ACKs
                    recipients[i].paymentRequest = request;
                recipients[i].address = QString::fromStdString(CBitcoinAddress(dest).ToString());
                if (fDebug)
                    qDebug() << "PaymentServer::processPaymentRequest : Payment request, insecure " << recipients[i].address;
            }
            else {
                // Insecure payments to custom lycancoin addresses are not supported
                // (there is no good way to tell the user where they are paying in a way
                // they'd have a chance of understanding).
                emit reportError(tr("Payment request error"), 
                                 tr("Insecure requests to custom payment scripts unsupported"),
                                 CClientUIInterface::MODAL);
                return false;
            }
        }
    }

    return true;
}

void
PaymentServer::fetchRequest(const QUrl& url)
{
    QNetworkRequest netRequest;
    netRequest.setAttribute(QNetworkRequest::User, "PaymentRequest");
    netRequest.setUrl(url);
    netRequest.setRawHeader("User-Agent", CLIENT_NAME.c_str());
    netRequest.setRawHeader("Accept", BITCOIN_REQUEST_MIMETYPE);
    netManager->get(netRequest);
}

void
PaymentServer::fetchPaymentACK(CWallet* wallet, SendCoinsRecipient recipient, QByteArray transaction)
{
    const payments::PaymentDetails& details = recipient.paymentRequest.getDetails();
    if (!details.has_payment_url())
        return;

    QNetworkRequest netRequest;
    netRequest.setAttribute(QNetworkRequest::User, "PaymentACK");
    netRequest.setUrl(QString::fromStdString(details.payment_url()));
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/lycancoin-payment");
    netRequest.setRawHeader("User-Agent", CLIENT_NAME.c_str());
    netRequest.setRawHeader("Accept", BITCOIN_PAYMENTACK_MIMETYPE);

    payments::Payment payment;
    payment.set_merchant_data(details.merchant_data());
    payment.add_transactions(transaction.data(), transaction.size());

    // Create a new refund address, or re-use:
    QString account = tr("Refund from") + QString(" ") + recipient.authenticatedMerchant;
    std::string strAccount = account.toStdString();
    set<CTxDestination> refundAddresses = wallet->GetAccountAddresses(strAccount);
    if (!refundAddresses.empty()) {
        CScript s; s.SetDestination(*refundAddresses.begin());
        payments::Output* refund_to = payment.add_refund_to();
        refund_to->set_script(&s[0], s.size());
    }
    else {
        CPubKey newKey;
        if (wallet->GetKeyFromPool(newKey)) {
            CKeyID keyID = newKey.GetID();
            wallet->SetAddressBook(keyID, strAccount, "refund");

            CScript s; s.SetDestination(keyID);
            payments::Output* refund_to = payment.add_refund_to();
            refund_to->set_script(&s[0], s.size());
        }
        else {
            // This should never happen, because sending coins should have just unlocked the wallet
            // and refilled the keypool
            qDebug() << "PaymentServer::fetchPaymentACK : Error getting refund key, refund_to not set";
        }
    }

    int length = payment.ByteSize();
    netRequest.setHeader(QNetworkRequest::ContentLengthHeader, length);
    QByteArray serData(length, '\0');
    if (payment.SerializeToArray(serData.data(), length)) {
        netManager->post(netRequest, serData);
    }
    else {
        // This should never happen, either:
        qDebug() << "PaymentServer::fetchPaymentACK : Error serializing payment message";
    }
}

void
PaymentServer::netRequestFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
    {
        QString message = QObject::tr("Error communicating with %1: %2")
            .arg(reply->request().url().toString())
            .arg(reply->errorString());
        qDebug() << "PaymentServer::netRequestFinished : " << message;
        emit reportError(tr("Network request error"), message, CClientUIInterface::MODAL);
        return;
    }

    QByteArray data = reply->readAll();

    QString requestType = reply->request().attribute(QNetworkRequest::User).toString();
    if (requestType == "PaymentRequest")
    {
        PaymentRequestPlus request;
        QList<SendCoinsRecipient> recipients;
        if (request.parse(data) && processPaymentRequest(request, recipients)) {
            foreach (const SendCoinsRecipient& recipient, recipients){
                emit receivedPaymentRequest(recipient);
            }
        }
        else
            qDebug() << "PaymentServer::netRequestFinished : Error processing payment request";
        return;
    }
    else if (requestType == "PaymentACK")
    {
        payments::PaymentACK paymentACK;
        if (!paymentACK.ParseFromArray(data.data(), data.size()))
        {
            QString message = QObject::tr("Bad response from server %1")
                .arg(reply->request().url().toString());
            qDebug() << "PaymentServer::netRequestFinished : " << message;
            emit reportError(tr("Network request error"), message, CClientUIInterface::MODAL);
        }
        else {
            emit receivedPaymentACK(QString::fromStdString(paymentACK.memo()));
        }
    }
}

void
PaymentServer::reportSslErrors(QNetworkReply* reply, const QList<QSslError> &errs)
{
    QString errString;
    foreach (const QSslError& err, errs) {
        qDebug() << "PaymentServer::reportSslErrors : " << err;
        errString += err.errorString() + "\n";
    }
    emit reportError(tr("Network request error"), errString, CClientUIInterface::MODAL);
}

void PaymentServer::setOptionsModel(OptionsModel *optionsModel)
{
    this->optionsModel = optionsModel;
}