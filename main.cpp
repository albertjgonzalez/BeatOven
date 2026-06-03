#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <iostream>
#include <filesystem>
#include <vector>
#include <QtNetwork/QTcpSocket>
#include <QTcpServer>
#include <QJsonObject>
#include <QTimer>


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

std::vector<std::filesystem::path> getLocalProjects() {
	std::vector<std::filesystem::path> localProjectsVector;
    auto localProjects = std::filesystem::path("D:/Music/26$$");
	if(std::filesystem::is_directory(localProjects)) {
		std::cout << localProjects.relative_path() << " is a directory" << std::endl;

		for (auto const& dir_entry : std::filesystem::directory_iterator{localProjects}) {
			std::cout << "Current Projects: " << std::endl;
        		std::cout << dir_entry.path() << '\n';
			localProjectsVector.push_back(dir_entry.path());
		}

	}
	else
		std::cout << localProjects.relative_path() << " is a not directory" << std::endl;

	return localProjectsVector;
}

int main(int argc, char *argv[]) {
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
    });


    //auto localProjects = getLocalProjects();
    //sendLocalProjects(localProjects);
	


    //QWidget window;
    //window.setWindowTitle("Hello Qt");
    //window.resize(400, 300);

    // Add a label inside a layout
    //QVBoxLayout *layout = new QVBoxLayout(&window);
    //QLabel *label = new QLabel("Hello, World!");
    //label->setAlignment(Qt::AlignCenter);
    //layout->addWidget(label);

    //window.show();

    return app.exec();
}
