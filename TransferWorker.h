#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H
#include <QTcpSocket>
#include "Config.h"

class TransferWorker : public QObject
{
    Q_OBJECT
public:
    QTcpSocket* mSocket;
    Config& cfg;

    TransferWorker(QTcpSocket* socket, Config& cfg);

public slots:
    void doTransfer();

signals:
    void progress(qint64 cBytes);
    void finished();
};

#endif // TRANSFERWORKER_H
