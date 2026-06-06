#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#endif // CONFIG_H
struct Config {
    std::string LocalProjectsDirectory;
    std::string SharedProjectsDirectory;
    std::string connectString;
};
