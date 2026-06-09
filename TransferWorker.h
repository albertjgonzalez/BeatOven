#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H
#include "Config.h"
#include "BeatOvenServer.h"
#include <QTcpSocket>

class TransferWorker : public QObject
{
    Q_OBJECT
public:
    QTcpSocket* mSocket;
    Config& cfg;
    BeatOvenServer* mServer;

    TransferWorker(QTcpSocket* socket, Config& cfg, BeatOvenServer* server);

public slots:
    void doTransfer();

signals:
    void progress(qint64 cBytes);
    void finished();
};

#endif // TRANSFERWORKER_H
