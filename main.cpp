#include "BeatOvenClient.h"
#include "BeatOvenServer.h"
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

int main(int argc, char *argv[]) {
    //App backend setup section
    QApplication app(argc, argv);
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
