#include "TransferWorker.h"
#include "BeatOvenServer.h"
#include "ProjectFiles.h"
#include <iostream>

TransferWorker::TransferWorker(QTcpSocket* socket, Config& cfg,  BeatOvenServer* server) :
    mSocket{socket}, cfg{cfg}, mServer{server}
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

    auto header = initHeaderFromClient.toStdString();

    std::vector<projectFiles> projectFilesVector;

    size_t cursor = 1;
    size_t hash = header.find('#', cursor);
    int projectCount = std::stoi(std::string(header.substr(cursor, hash - cursor)));
    cursor = hash + 1;

    for (int i = 0; i < projectCount; ++i) {
        size_t colon = header.find(':', cursor);
        size_t fieldEnd = header.find('#', colon);

        std::string name = std::string(header.substr(cursor, colon - cursor));
        qint64 size = std::stoll(std::string(header.substr(colon + 1, fieldEnd - colon - 1)));

        projectFilesVector.push_back({name, size});
        cursor = fieldEnd + 1;                  // advance past this field's '#'
    }

    mSocket->waitForBytesWritten();
    mSocket->isReadable();

    qint64 total = 0;
    for (const auto& f : projectFilesVector) total += f.size;

    QByteArray chunk;
    while (mSocket->waitForReadyRead()) {
        chunk += mSocket->readAll();
        qint64 pct = total ? chunk.size() * 100 / total : 0;
        std::cout << "\rServer Transfer Progress: " << pct << "%" << std::flush;
        emit progress(pct);
    }

    // std::cout << initHeaderFromClient.toStdString() << std::endl;
    // std::cout << "chunk size: " << chunk.size() << std::endl;
    mServer->createFilesFromTransfer(projectFilesVector, chunk);

    emit finished();
}
