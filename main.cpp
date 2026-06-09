#include "BeatOvenClient.h"
#include "BeatOvenServer.h"
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
#include <QGridLayout>
#include <QListWidget>
#include <QListWidgetItem>

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

    QWidget window;
    window.setWindowTitle("Hello Qt");
    window.resize(400, 300);

    // Add a label inside a layout
    // QLabel *label = new QLabel("Hello, World!");
    //label->setAlignment(Qt::AlignCenter);
    //layout->addWidget(label);

    QGridLayout *layout = new QGridLayout(&window);

    QPushButton *button = new QPushButton("&Share");

    QProgressBar *progressBar = new QProgressBar;
    progressBar->setRange(0,100);

    QListWidget *projectBox = new QListWidget();
    projectBox->setViewMode(QListView::IconMode);

    auto localProjectsDirectory = std::filesystem::path(config.LocalProjectsDirectory);
    auto projectsList = getLocalProjectsVector(localProjectsDirectory);
    QIcon icon("C:/Users/Left Ear/source/repos/BeatOvenHost/ProjectCube.png");

    for (const auto& project : projectsList) {
        QListWidgetItem *projectIcon = new QListWidgetItem();
        projectIcon->setText(QString::fromStdString(project.filename().string()));
        projectIcon->setIcon(icon);
        projectBox->addItem(projectIcon);
    }



    layout->addWidget(button,      0,0);
    layout->addWidget(progressBar, 1,0);
    layout->addWidget(projectBox,  2,0);


    std::string projectName = {"26-1.2 Project"};
    auto projectToTransfer = getProjectForTransfer(config, projectName);

    QObject::connect(button, &QPushButton::clicked, [&projectToTransfer, &config, progressBar](){
        sendLocalProject(projectToTransfer, config, progressBar);
    });

    window.show();

    return app.exec();
}
