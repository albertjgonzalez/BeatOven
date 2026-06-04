#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <QtNetwork/QTcpSocket>
#include <QTcpServer>
#include <QJsonObject>
#include <QTimer>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>

struct Config {
    std::string LocalProjectsDirectory;
    std::string connectString;
};

void setConfigValues(Config& cfg) {
    std::cout << "Setting Config Values" << std::endl;
    auto appPath = QCoreApplication::applicationDirPath().toStdString();
    std::filesystem::path configLocation = std::filesystem::path(appPath) / "beatoven.conf";
    std::ifstream readconfig(configLocation);
    if (!readconfig.is_open()) {
        std::cout << "Error: config file could not open." << std::endl;
        return;
    }
    std::string configValue;
    while (std::getline(readconfig, configValue)) {
        auto pos = configValue.find('=');
        if (pos == std::string::npos) continue;

        std::string key = configValue.substr(0, pos);
        std::string value = configValue.substr(pos+1);
        //if (key=="LocalProjectsDirectory") {
        if (key == "TempProjectsDirectory") { // for testing
            cfg.LocalProjectsDirectory = value;
        }
        if (key == "ConnectionString") {
            cfg.connectString = value;
        }
    }
}

void sendLocalProjects(const std::vector<std::filesystem::path>& localProjects, Config& cfg) {
	QTcpSocket socket;
    QString hostName = QString::fromStdString(cfg.connectString);
    quint16 port {8000};

	socket.connectToHost(hostName,port);

	if (!socket.isValid()) std::cout << "Socket is not valid" << std::endl;
	
	std::cout << "Sending Local Projects: " << std::endl;
    if (socket.waitForConnected()) {
        std::cout << "Socket Ready for connection" << std::endl;

        if (socket.waitForReadyRead()) {
            std::cout << "Socket waiting for read" << std::endl;

            //eventually have token to validate session
            QByteArray token = socket.readAll();
            std::cout << token.toStdString() << std::endl;

            //create header info
            std::string header {"header"};
            int count{0};

            // for (const auto& p : localProjects) {
            //     ++count;
            //     QFile projectFolder = QFile(p);
            // }

            //send header -> amount of projects, other meta info
            //header.append(std::to_string(count));

            socket.write(header.c_str(), header.size());
             socket.waitForBytesWritten();

            // for (const auto& p : localProjects) {

            //     QFile projectFile = QFile(p);
            //     if (!projectFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //         std::cout << "Error: could not open project: " << p.filename() << std::endl;
            //         return;
            //     }


            //     while (!projectFile.atEnd()) {
            //         QByteArray line = projectFile.readLine();
            //         std::cout << line.toStdString() << std::endl;
            //         socket.write(line);
            //         socket.waitForBytesWritten();
            //     }
            //     //std::cout << p.relative_path() << std::endl;
            // }

            //all of projects sent
            // std::string completedMessage {"this is the end bye."};

            // socket.write(completedMessage.c_str());
            //socket.waitForBytesWritten();
        }
        else {
            std::cout << "Error: " << socket.error() << std::endl;
        }
    }
}

std::vector<std::filesystem::path> getLocalProjects(Config& cfg) {
	std::vector<std::filesystem::path> localProjectsVector;
    auto localProjects = std::filesystem::path(cfg.LocalProjectsDirectory);
	if(std::filesystem::is_directory(localProjects)) {
		std::cout << localProjects.relative_path() << " is a directory" << std::endl;

        //std::cout << "Current Projects: " << std::endl;
		for (auto const& dir_entry : std::filesystem::directory_iterator{localProjects}) {
                //std::cout << dir_entry.path() << '\n';
			localProjectsVector.push_back(dir_entry.path());
		}

	}
	else
		std::cout << localProjects.relative_path() << " is a not directory" << std::endl;

	return localProjectsVector;
}

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

    QObject::connect(&server, &QTcpServer::newConnection, [&server](){
        std::cout << "Connection Made." << std::endl;
        auto socket = server.nextPendingConnection();
        if (!socket) { std::cout << "null socket" << std::endl; return; }

        socket->write("Hello\n");
        socket->waitForBytesWritten(30000);
        socket->waitForReadyRead();
        QByteArray text = socket->readAll();
            std::cout << text.toStdString() << std::endl;
        //while (socket->canReadLine()) {
        //    QByteArray text = socket->readLine();
        //    std::cout << text.toStdString() << std::endl;
        //}

    });

    auto localProjects = getLocalProjects(config);
    //sendLocalProjects(localProjects, config);


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
