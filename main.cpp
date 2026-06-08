#include "BeatOvenClient.h"
#include "BeatOvenServer.h"
#include "SendWorker.h"
#include <QThread>
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <iostream>
#include <QtNetwork/QTcpSocket>
#include <QTcpServer>
#include <QJsonObject>
#include <QTimer>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>
#include <QDir>
#include <QDataStream>
#include <QProgressBar>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Config config;

    setConfigValues(config);

    quint16 port {8000};
    QTcpServer server;

    std::cout << "Starting Server.." << std::endl;
    if (!server.listen(QHostAddress::Any, port)) {
        auto e = server.errorString();
        std::cout << e.toStdString() << std::endl;
    }

    QObject::connect(&server, &QTcpServer::newConnection,[&server, &config](){
        runServer(server, config);
    });

    auto localProjects = getLocalProjects(config);

    QWidget window;
    window.setWindowTitle("Hello Qt");
    window.resize(400, 300);

    // Add a label inside a layout
    // QLabel *label = new QLabel("Hello, World!");
    //label->setAlignment(Qt::AlignCenter);
    //layout->addWidget(label);

    QVBoxLayout *layout = new QVBoxLayout(&window);

    QPushButton *button = new QPushButton("&Share");

    QProgressBar *progressBar = new QProgressBar;
    progressBar->setRange(0,100);

    layout->addWidget(button);
    layout->addWidget(progressBar);

    QObject::connect(button, &QPushButton::clicked, [&localProjects, &config, progressBar](){
        auto worker = new SendWorker(localProjects, config);
        auto thread = new QThread;
        worker->moveToThread(thread);

        QObject::connect(thread, &QThread::started, worker, &SendWorker::doSend);
        auto c = QObject::connect(worker, &SendWorker::progress, progressBar, [progressBar](qint64 pct){
            std::cout << "UI slot got: " << pct << std::endl;
            progressBar->setValue(static_cast<int>(pct));
        });
        std::cout << "connected: " << (bool)c << std::endl;
        QObject::connect(worker, &SendWorker::finished, thread, &QThread::quit);
        QObject::connect(worker, &SendWorker::finished, worker, &SendWorker::deleteLater);
        QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        thread->start();
    });

    window.show();

    return app.exec();
}
