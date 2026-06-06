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
#include <string_view>
#include <QDir>


struct Config {
    std::string LocalProjectsDirectory;
    std::string SharedProjectsDirectory;
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

        if (key == "TempProjectsDirectory") { // for testing
            cfg.LocalProjectsDirectory = value;
        }
        if (key == "ConnectionString") {
            cfg.connectString = value;
        }
        if (key == "LocalSharedProjectsDirectory") {
            cfg.SharedProjectsDirectory = value;
        }
    }
}

void createFilesFromTransfer(const Config& cfg, std::string_view header, const QByteArray& data) {
    struct projectFiles{ std::string name; qint64 size; };
    std::vector<projectFiles> projectFilesVector;

    size_t cursor = 1;                          // skip leading '#'
    size_t hash = header.find('#', cursor);
    int projectCount = std::stoi(std::string(header.substr(cursor, hash - cursor)));
    cursor = hash + 1;                          // now at first file field

    for (int i = 0; i < projectCount; ++i) {
        size_t colon = header.find(':', cursor);
        size_t fieldEnd = header.find('#', colon);

        std::string name = std::string(header.substr(cursor, colon - cursor));
        qint64 size = std::stoll(std::string(header.substr(colon + 1, fieldEnd - colon - 1)));

        projectFilesVector.push_back({name, size});
        cursor = fieldEnd + 1;                  // advance past this field's '#'
    }

    std::ofstream output;
    qint64 dataOffset{0};
    QDir QsharedDir = QString(cfg.SharedProjectsDirectory.c_str());

    for (const auto& f: projectFilesVector) {
        auto fileBytes = data.mid(dataOffset, f.size);

        QString fullPath = QsharedDir.filePath(f.name.c_str());
        QDir().mkpath(QFileInfo(fullPath).absolutePath());

        QFile out(fullPath);
        if (!out.open(QIODevice::WriteOnly)) {
            std::cout << "Server Error: could not write " << f.name << std::endl;
            return;
        }
        out.write(fileBytes);
        out.close();

        dataOffset += f.size;
    }

    std::vector<std::string> projectNamesVector;
    std::vector<QByteArray> projectBytesVector;

    size_t start = 0;

}

void sendLocalProjects(const std::vector<std::filesystem::path>& localProjects, Config& cfg) {
	QTcpSocket socket;
    QString hostName = QString::fromStdString(cfg.connectString);
    quint16 port {8000};

	socket.connectToHost(hostName,port);

	if (!socket.isValid()) std::cout << "Socket is not valid" << std::endl;
	
    std::cout << "Client: Sending Local Projects: " << std::endl;
    if (socket.waitForConnected()) {
        std::cout << "Client: Socket Ready for connection" << std::endl;

        //create header info
        std::string header {"#"};
        header += std::to_string(localProjects.size());

        for (const auto& p : localProjects) {
            std::cout << "Client: file: " << p.string() << std::endl;
            auto pSize = std::filesystem::file_size(std::filesystem::path(cfg.LocalProjectsDirectory) / p);
            header += "#" + p.string() + ":" + std::to_string(pSize);
        }

        //send header -> amount of projects, other meta info
        QByteArray headerBytes = QByteArray::fromStdString(header);
        QDataStream stream(&socket);
        stream << headerBytes;   // writes length, then bytes
        socket.waitForBytesWritten();

        if (socket.waitForReadyRead()) {
            std::cout << "Client: Socket waiting for read" << std::endl;
            QByteArray response = socket.readAll();
            std::cout << "Client: " << response.toStdString() << std::endl;
        }

        for (const auto& p : localProjects) {

            QFile projectFile = QFile(p);
            if (!projectFile.open(QIODevice::ReadOnly)) {
                std::cout << "Client Error: could not open project: " << p.filename() << std::endl;
                return;
            }

            while (!projectFile.atEnd()) {
                QByteArray block = projectFile.read(64 * 1024);
                socket.write(block);
                socket.waitForBytesWritten();
            }
        }
    }
}

std::vector<std::filesystem::path> createProjectSubDirectoryVector(const std::filesystem::path& D) {
    std::vector<std::filesystem::path> subD;
    if (!std::filesystem::is_directory(D)) {
        //std::cout << "Dir name: " << D.filename() << std::endl;
        subD.push_back(D);
        return subD;
    }

    try {
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(D)) {
            if(std::filesystem::is_directory(dir_entry))
                continue;

            subD.push_back(std::filesystem::relative(dir_entry, D));
            //std::cout << "Client: File Name is " << std::filesystem::relative(dir_entry, D) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return subD;
}

std::vector<std::filesystem::path> getLocalProjects(Config& cfg) {
	std::vector<std::filesystem::path> localProjectsVector;
    auto localProjects = std::filesystem::path(cfg.LocalProjectsDirectory);
	if(std::filesystem::is_directory(localProjects)) {
		std::cout << localProjects.relative_path() << " is a directory" << std::endl;

        auto mappedProjectsVector = createProjectSubDirectoryVector(localProjects);
        return mappedProjectsVector;
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

    QObject::connect(&server, &QTcpServer::newConnection, [&server, config](){
        std::cout << "Server: Connection Made." << std::endl;
        auto socket = server.nextPendingConnection();
        if (!socket) { std::cout << "null socket" << std::endl; return; }

        socket->waitForReadyRead();

        QDataStream stream(socket);
        QByteArray initHeaderFromClient;
        stream.startTransaction();
        stream >> initHeaderFromClient;
        while (!stream.commitTransaction()) {
            socket->waitForReadyRead();
            stream.startTransaction();
            stream >> initHeaderFromClient;
        }

        socket->write("Recieved Header\n");
        socket->waitForBytesWritten();
        socket->isReadable();
        QByteArray chunk;
        while (socket->waitForReadyRead()) {
            chunk += socket->readAll();
        }
        std::cout << initHeaderFromClient.toStdString() << std::endl;
        createFilesFromTransfer(config, initHeaderFromClient.toStdString(), chunk);
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
