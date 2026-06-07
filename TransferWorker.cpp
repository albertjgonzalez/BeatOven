#include "TransferWorker.h"
#include <iostream>
#include "BeatOvenServer.h"

TransferWorker::TransferWorker(QTcpSocket* socket, Config& cfg) :
                                mSocket{socket}, cfg(cfg)
{}

void TransferWorker::doTransfer() {
    std::cout << "Server: Connection Made." << std::endl;
    if (!mSocket) { std::cout << "null socket" << std::endl; return; }

    mSocket->waitForReadyRead();

    QDataStream stream(mSocket);
    QByteArray initHeaderFromClient;
    stream.startTransaction();
    stream >> initHeaderFromClient;
    while (!stream.commitTransaction()) {
        mSocket->waitForReadyRead();
        stream.startTransaction();
        stream >> initHeaderFromClient;
    }

    mSocket->write("Recieved Header\n");
    mSocket->waitForBytesWritten();
    mSocket->isReadable();
    QByteArray chunk;
    while (mSocket->waitForReadyRead()) {
        chunk += mSocket->readAll();
    }
    std::cout << initHeaderFromClient.toStdString() << std::endl;
    std::cout << "chunk size: " << chunk.size() << std::endl;
    createFilesFromTransfer(cfg, initHeaderFromClient.toStdString(), chunk);

    emit finished();
}
