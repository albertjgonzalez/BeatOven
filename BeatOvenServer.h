#ifndef BEATOVENSERVER_H
#define BEATOVENSERVER_H
#include <QTcpServer>
#include "Config.h"
#include "ProjectFiles.h"

void createFilesFromTransfer(const Config& cfg, std::vector<projectFiles>& projectFilesVector, const QByteArray& data);

void runServer(QTcpServer& server, Config& cfg);
#endif // BEATOVENSERVER_H
