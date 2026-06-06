#ifndef CONFIG_H
#define CONFIG_H
#include <string>
struct Config {
    std::string LocalProjectsDirectory;
    std::string SharedProjectsDirectory;
    std::string connectString;
};
void setConfigValues(Config& cfg);
#endif // CONFIG_H

