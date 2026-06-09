#include "Config.h"
#include <iostream>
#include <QCoreApplication>
#include <filesystem>
#include <fstream>

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
        if (key == "LocalSharedProjectsDirectory") {
            cfg.SharedProjectsDirectory = value;
        }
        if (key == "ConnectionString") {
            cfg.connectString = value;
        }
        if (key == "Port") {
            cfg.Port = std::stoi(value);
        }
    }
}
