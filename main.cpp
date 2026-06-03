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

struct Config {
    std::string LocalProjectsDirectory;
};

void setConfigValues(Config& cfg) {
    std::cout << "Setting Config Values" << std::endl;
    std::filesystem::path configLocation = "./beatoven.conf";
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
        if (key=="LocalProjectsDirectory") {
            cfg.LocalProjectsDirectory = value;
        }
    }
}

void sendLocalProjects(const std::vector<std::filesystem::path>& localProjects) {
	QTcpSocket socket;
    QString hostName {"192.0.0.1"};
    quint16 port {8000};

	socket.connectToHost(hostName,port);

	if (!socket.isValid()) std::cout << "Socket is not valid" << std::endl;
	
	std::cout << "Sending Local Projects: " << std::endl;
	for (const auto& p : localProjects) {
		std::cout << p.relative_path() << std::endl;
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

    Config config;

    setConfigValues(config);

    QApplication app(argc, argv);

    quint16 port {8000};
    QTcpServer server;

    std::cout << "Starting Server.." << std::endl;
    if (!server.listen(QHostAddress::Any, port)) {
        auto e = server.errorString();
        std::cout << "e.toStdString()" << std::endl;
    }

    QObject::connect(&server, &QTcpServer::newConnection, [&server](){
        std::cout << "Connection Made-------------" << std::endl;
        auto socket = server.nextPendingConnection();
        socket->write("Hello\n");
        socket->waitForBytesWritten(30000);
    });


    auto localProjects = getLocalProjects(config);
    //sendLocalProjects(localProjects);
	


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

    QObject::connect(button, &QPushButton::clicked, [&localProjects](){
            sendLocalProjects(localProjects);
    });

    window.show();

    return app.exec();
}
