#ifndef CLIENTMODEL_H
#define CLIENTMODEL_H

#include <QObject>
#include <QDateTime>

class AddressTableModel;
class OptionsModel;
class PeerTableModel;
class TransactionTableModel;

class CWallet;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

enum BlockSource {
    BLOCK_SOURCE_NONE,
    BLOCK_SOURCE_REINDEX,
    BLOCK_SOURCE_DISK,
    BLOCK_SOURCE_NETWORK
};

enum NumConnections {
    CONNECTIONS_NONE = 0,
    CONNECTIONS_IN   = (1U << 0),
    CONNECTIONS_OUT  = (1U << 1),
    CONNECTIONS_ALL  = (CONNECTIONS_IN | CONNECTIONS_OUT),
};

/** Model for Lycancoin network client. */
class ClientModel : public QObject
{
    Q_OBJECT

public:
    explicit ClientModel(OptionsModel *optionsModel, QObject *parent = 0);
    ~ClientModel();

    OptionsModel *getOptionsModel();
    PeerTableModel *getPeerTableModel();

    //! Return number of connections, default is in- and outbound (total)
    int getNumConnections(unsigned int flags = CONNECTIONS_ALL) const;
    int getNumBlocks() const;
    
    quint64 getTotalBytesRecv() const;
    quint64 getTotalBytesSent() const;

    double getVerificationProgress() const;
    QDateTime getLastBlockDate() const;

    //! Return true if core is doing initial block download
    bool inInitialBlockDownload() const;
    //! Return true if core is importing blocks
    enum BlockSource getBlockSource() const;
    //! Return warnings to be displayed in status bar
    QString getStatusBarWarnings() const;

    QString formatFullVersion() const;
    QString formatBuildDate() const;
    bool isReleaseVersion() const;
    QString clientName() const;
    QString formatClientStartupTime() const;

private:
    OptionsModel *optionsModel;
    PeerTableModel *peerTableModel;

    int cachedNumBlocks;
    QDateTime cachedBlockDate;
    bool cachedReindexing;
    bool cachedImporting;

    QTimer *pollTimer;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

signals:
    void numConnectionsChanged(int count);
    void numBlocksChanged(int count, const QDateTime& blockDate);
    void alertsChanged(const QString &warnings);
    void bytesChanged(quint64 totalBytesIn, quint64 totalBytesOut);

    //! Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Show progress dialog e.g. for verifychain
    void showProgress(const QString &title, int nProgress);

public slots:
    void updateTimer();
    void updateNumConnections(int numConnections);
    void updateAlert(const QString &hash, int status);
};

#endif // CLIENTMODEL_H