#ifndef BEATOVENSERVER_H
#define BEATOVENSERVER_H
#include <QTcpServer>
#include "Config.h"

void createFilesFromTransfer(const Config& cfg, std::string_view header, const QByteArray& data);

void runServer(QTcpServer& server, Config& cfg);
#endif // BEATOVENSERVER_H
