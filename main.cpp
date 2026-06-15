#include "BeatOvenClient.h"
#include "BeatOvenServer.h"
#include <iostream>
#include <QThread>
#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
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
#include <QMessageBox>
#include <QSslSocket>

int main(int argc, char *argv[]) {
    //App backend setup section
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("BeatOven");

    //OpenSSL TLS backend
    if (!QSslSocket::availableBackends().contains(QStringLiteral("openssl"))) {
        std::cout << "FATAL: Qt OpenSSL TLS plugin unavailable. Backends found: "
                  << QSslSocket::availableBackends().join(", ").toStdString() << std::endl;
        QMessageBox::critical(nullptr, "BeatOven",
                              "The OpenSSL TLS backend for Qt is not installed. Secure transfers cannot work.");
        return 1;
    }
    QSslSocket::setActiveBackend(QStringLiteral("openssl"));
    if (!QSslSocket::supportsSsl()) {
        std::cout << "FATAL: OpenSSL runtime libraries not found at runtime." << std::endl;
        QMessageBox::critical(nullptr, "BeatOven",
                              "OpenSSL libraries could not be loaded. On Windows, place libssl-3-x64.dll and "
                              "libcrypto-3-x64.dll next to BeatOven.exe.");
        return 1;
    }
    std::cout << "TLS backend: " << QSslSocket::activeBackend().toStdString()
              << " (" << QSslSocket::sslLibraryVersionString().toStdString() << ")" << std::endl;


    // Identity identity = Identity::loadOrCreate();
    // if (!identity.isValid()) {
    //     QMessageBox::critical(nullptr, "BeatOven",
    //                           "Could not load or create the peer identity. See console output.");
    //     return 1;
    // }

    Config config;
    setConfigValues(config);

    //App frontend setup section
    QWidget window;
    window.setWindowTitle("Hello Qt");
    window.resize(400, 300);

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
        projectIcon->setData(Qt::UserRole, QString::fromStdString(project.filename().string()));
        projectBox->addItem(projectIcon);
    }

    layout->addWidget(button,      0,0);
    layout->addWidget(progressBar, 1,0);
    layout->addWidget(projectBox,  2,0);

    //Launch App server
    BeatOvenServer *appServer = new BeatOvenServer(config);

    QObject::connect(projectBox, &QListWidget::itemClicked, [layout, &config, progressBar](QListWidgetItem* item){
        std::string name = item->data(Qt::UserRole).toString().toStdString();

        auto reply = QMessageBox::question(nullptr, "Confirm Transfer",
                                           QString::fromStdString("Send project: " + name + "?"));

        if (reply == QMessageBox::Yes) {
            auto projectToTransfer = getProjectForTransfer(config, name);
            sendLocalProject(projectToTransfer, config, progressBar);
        }
    });

    window.show();

    return app.exec();
}
