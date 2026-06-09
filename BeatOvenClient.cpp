#include "BeatOvenClient.h"
#include "SendWorker.h"
#include <vector>
#include <filesystem>
#include <iostream>
#include <QTcpSocket>
#include <QFile>
#include <QThread>
#include <QProgressBar>

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

            subD.push_back(std::filesystem::relative(dir_entry, D.parent_path()));
            //std::cout << "Client: File Name is " << std::filesystem::relative(dir_entry, D) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return subD;
}

std::vector<std::filesystem::path> getProjectForTransfer(Config& cfg, std::string_view projectName) {
    std::vector<std::filesystem::path> localProjectsVector;
    auto localProjects = std::filesystem::path(cfg.LocalProjectsDirectory);
    if(std::filesystem::is_directory(localProjects)) {
        std::cout << localProjects.relative_path() << " is a directory" << std::endl;
        std::vector<std::filesystem::path> mappedProjectsVector;

        for (auto const& project : std::filesystem::directory_iterator(localProjects)) {
            if (std::filesystem::is_directory(project) && std::filesystem::path(project).filename() == projectName) {
                mappedProjectsVector = createProjectSubDirectoryVector(project);
                return mappedProjectsVector;
            }
        }

        if (mappedProjectsVector.empty())
            std::cout << "Error: Could not locate " << projectName << std::endl;
    }
    else
        std::cout << localProjects.relative_path() << " is a not directory" << std::endl;

    return localProjectsVector;
}

std::vector<std::filesystem::path> getLocalProjectsVector(std::filesystem::path& projectsFolder) {
    std::vector<std::filesystem::path> localProjectsFolder;

    if (!std::filesystem::is_directory(projectsFolder))
        std::cout << projectsFolder.filename() << " is not a directory" << std::endl;

    for (const auto& project : std::filesystem::directory_iterator(projectsFolder)) {
        localProjectsFolder.push_back(project);
    }

    return localProjectsFolder;
}

void sendLocalProject(const std::vector<std::filesystem::path>& localProjects, Config& cfg, QProgressBar* progressBar) {
    auto worker = new SendWorker(localProjects, cfg);
    auto thread = new QThread();
    worker->moveToThread(thread);

    QObject::connect(thread, &QThread::started, worker, &SendWorker::doSend);
    QObject::connect(worker, &SendWorker::progress, progressBar, [progressBar](qint64 pct){
        progressBar->setValue(static_cast<int>(pct));
    });
    QObject::connect(worker, &SendWorker::finished, thread, &QThread::quit);
    QObject::connect(worker, &SendWorker::finished, worker, &SendWorker::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

}
