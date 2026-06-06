#include "BeatOvenServer.h"
#include <iostream>
#include <fstream>
#include <QTcpSocket>
#include <QDir>

void createFilesFromTransfer(const Config& cfg, std::string_view header, const QByteArray& data) {
    struct projectFiles{ std::string name; qint64 size; };
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


void runServer(QTcpServer& server,  Config& cfg) {
    std::cout << "Server: Connection Made." << std::endl;
    auto socket = server.nextPendingConnection();
    if (!socket) { std::cout << "null socket" << std::endl; return; }

    socket->waitForReadyRead();

    QDataStream stream(socket);
    QByteArray initHeaderFromClient;
    stream.startTransaction();
    stream >> initHeaderFromClient;
    while (!stream.commitTransaction()) {
        socket->waitForReadyRead();
        stream.startTransaction();
        stream >> initHeaderFromClient;
    }

    socket->write("Recieved Header\n");
    socket->waitForBytesWritten();
    socket->isReadable();
    QByteArray chunk;
    while (socket->waitForReadyRead()) {
        chunk += socket->readAll();
    }
    std::cout << initHeaderFromClient.toStdString() << std::endl;
    std::cout << "chunk size: " << chunk.size() << std::endl;
    createFilesFromTransfer(cfg, initHeaderFromClient.toStdString(), chunk);
}

