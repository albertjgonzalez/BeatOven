#ifndef SENDWORKER_H
#define SENDWORKER_H
#include "Config.h"
#include <filesystem>
#include <QObject>

class SendWorker : public QObject
{
    Q_OBJECT
public:
    std::vector<std::filesystem::path> mProjects;
    Config& cfg;
    SendWorker(std::vector<std::filesystem::path> projects, Config& cfg);
public slots:
    void doSend();
signals:
    void progress(qint64 pct);
    void finished();
};
#endif // SENDWORKER_H
