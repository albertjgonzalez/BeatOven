#ifndef BEATOVENSERVER_H
#define BEATOVENSERVER_H
#include "Config.h"
#include "ProjectFiles.h"
#include <QTcpServer>

class BeatOvenServer {

public:

    QTcpServer* mServer;
    Config& cfg;

    BeatOvenServer(Config& config);
    void runServer();
    void createFilesFromTransfer(std::vector<projectFiles>& projectFilesVector, const QByteArray& data);
};
#endif // BEATOVENSERVER_H
