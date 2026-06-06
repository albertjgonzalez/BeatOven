#ifndef BEATOVENSERVER_H
#define BEATOVENSERVER_H
#include "Config.h"
#include <QTcpServer>
#endif // BEATOVENSERVER_H

void createFilesFromTransfer(const Config& cfg, std::string_view header, const QByteArray& data);

void runServer(QTcpServer& server, Config& cfg);
