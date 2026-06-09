#ifndef BEATOVENCLIENTH_H
#define BEATOVENCLIENTH_H
#include "Config.h"
#include <vector>
#include <filesystem>
#include <QProgressBar>

std::vector<std::filesystem::path> createProjectSubDirectoryVector(const std::filesystem::path& D);

std::vector<std::filesystem::path> getProjectForTransfer(Config& cfg, std::string_view projectName);

std::vector<std::filesystem::path> getLocalProjectsVector(std::filesystem::path& projectsFolder);

void sendLocalProject(const std::vector<std::filesystem::path>& localProjects, Config& cfg, QProgressBar* progressBar);

#endif // BEATOVENCLIENTH_H
