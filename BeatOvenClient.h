#ifndef BEATOVENCLIENT_H
#define BEATOVENCLIENT_H
#include <vector>
#include <filesystem>
#include "Config.h"

std::vector<std::filesystem::path> createProjectSubDirectoryVector(const std::filesystem::path& D);

std::vector<std::filesystem::path> getLocalProjects(Config& cfg);

void sendLocalProjects(const std::vector<std::filesystem::path>& localProjects, Config& cfg);

#endif // BEATOVENCLIENT_H
