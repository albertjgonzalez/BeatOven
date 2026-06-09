#include "BeatOvenServer.h"
#include "TransferWorker.h"
#include "ProjectFiles.h"
#include <iostream>
#include <fstream>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QThread>

BeatOvenServer::BeatOvenServer(Config& config) :
    mServer{ new QTcpServer }, cfg{ config }
{
    std::cout << "Starting Server.." << std::endl;

    if (!mServer->listen(QHostAddress::Any, (quint16)cfg.Port)) {
        auto e = mServer->errorString();
        std::cout << e.toStdString() << std::endl;
    }
    std::cout << mServer->serverAddress().toString().toStdString() << std::endl;
    std::cout << "listening on port: " << mServer->serverPort()  << std::endl;
    QObject::connect(mServer, &QTcpServer::newConnection, [this](){
        runServer();
    });
}

void BeatOvenServer::createFilesFromTransfer(std::vector<projectFiles>& projectFilesVector, const QByteArray& data) {
    std::ofstream output;
    qint64 dataOffset{0};
    QDir QsharedDir = QString(cfg.SharedProjectsDirectory.c_str());

    for (const auto& f: projectFilesVector) {
        auto fileBytes = data.mid(dataOffset, f.size);

        QString fullPath = QsharedDir.filePath(f.name.c_str());
        QDir().mkpath(QFileInfo(fullPath).absolutePath());

        QFile out(fullPath);
        if (!out.open(QIODevice::WriteOnly)) {
            std::cout << "Server Error: could not write " << f.name << std::endl;
            return;
        }
        out.write(fileBytes);
        out.close();

        dataOffset += f.size;
    }

    std::vector<std::string> projectNamesVector;
    std::vector<QByteArray> projectBytesVector;

    size_t start = 0;
}


void BeatOvenServer::runServer() {
    auto socket = mServer->nextPendingConnection();
    if (!socket){
        std::cout << "null Socket" <<std::endl;
        return;
    }
    socket->setParent(nullptr);

    auto tw = new TransferWorker(socket, cfg, this);
    auto thread = new QThread;
    tw->moveToThread(thread);
    socket->moveToThread(thread);

    QObject::connect(thread, &QThread::started, tw, &TransferWorker::doTransfer);
    QObject::connect(tw, &TransferWorker::finished, thread, &QThread::quit);
    QObject::connect(tw, &TransferWorker::finished, tw, &TransferWorker::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    QObject::connect(tw, &TransferWorker::finished, socket, &QTcpSocket::deleteLater);
    thread->start();
}

