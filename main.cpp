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
#include "BeatOvenClient.h"
#include "BeatOvenServer.h"


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
    QVBoxLayout *layout = new QVBoxLayout(&window);
    QLabel *label = new QLabel("Hello, World!");
    QPushButton *button = new QPushButton("&Share");

    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addWidget(button);

    QObject::connect(button, &QPushButton::clicked, [&localProjects, &config](){
            sendLocalProjects(localProjects, config);
    });

    window.show();

    return app.exec();
}
