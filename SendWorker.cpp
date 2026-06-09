#include "SendWorker.h"
#include "Config.h"
#include <filesystem>
#include <QTcpSocket>
#include <iostream>
#include <QFile>

SendWorker::SendWorker(std::vector<std::filesystem::path> projects, Config& cfg) :
                        cfg(cfg), mProjects(projects)
{}

void SendWorker::doSend() {
    QTcpSocket socket;
    QString hostName = QString::fromStdString(cfg.connectString);
    quint16 port {8000};

    socket.connectToHost(hostName,port);

    if (!socket.isValid()) std::cout << "Socket is not valid" << std::endl;

    std::cout << "Client: Sending Local Projects: " << std::endl;
    if (socket.waitForConnected()) {
        std::cout << "Client: Socket Ready for connection" << std::endl;

        //create header info
        std::string header {"#"};
        header += std::to_string(mProjects.size());

        for (const auto& p : mProjects) {
            //std::cout << "Client: file: " << p.string() << std::endl;
            auto pSize = std::filesystem::file_size(std::filesystem::path(cfg.LocalProjectsDirectory) / p);
            header += "#" + std::filesystem::path(p).generic_string() + ":" + std::to_string(pSize);
        }

        //send header -> amount of projects, other meta info
        QByteArray headerBytes = QByteArray::fromStdString(header);
        QDataStream stream(&socket);
        stream << headerBytes;   // writes length, then bytes
        socket.waitForBytesWritten();

        if (socket.waitForReadyRead()) {
            std::cout << "Client: Socket waiting for read" << std::endl;
            QByteArray response = socket.readAll();
            std::cout << "Client: " << response.toStdString() << std::endl;
        }

        qint64 total = 0;
        for (const auto& p : mProjects)
            total += std::filesystem::file_size(std::filesystem::path(cfg.LocalProjectsDirectory) / p);

        qint64 sent = 0;
        for (const auto& p : mProjects) {
            QString fullPath = QString::fromStdString((std::filesystem::path(cfg.LocalProjectsDirectory) / p).string());
            QFile projectFile = QFile(fullPath);

            if (!projectFile.open(QIODevice::ReadOnly)) {
                std::cout << "Client Error: could not open project: " << p.filename() << std::endl;
                return;
            }

            while (!projectFile.atEnd()) {
                QByteArray block = projectFile.read(64 * 1024);
                socket.write(block);
                socket.waitForBytesWritten();
                sent += block.size();
                qint64 pct = total ? 100 * sent / total : 0;
                //std::cout << "total bytes: " << total << std::endl;
                emit progress(pct);
            }
        }
        emit finished();
    } else {
        std::cout << "Connect failed: " << socket.errorString().toStdString()
        << " host=" << hostName.toStdString() << std::endl;
    }
}
