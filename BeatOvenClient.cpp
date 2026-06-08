#include "BeatOvenClient.h"
#include "SendWorker.h"
#include <vector>
#include <filesystem>
#include <iostream>
#include <QTcpSocket>
#include <QFile>
#include <QThread>

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

void sendLocalProjects(const std::vector<std::filesystem::path>& localProjects, Config& cfg) {
    auto worker = new SendWorker(localProjects, cfg);
    auto thread = new QThread();
    worker->moveToThread(thread);
    QObject::connect(thread, &QThread::started, worker, &SendWorker::doSend);
    QObject::connect(worker, &SendWorker::finished, thread, &QThread::quit);
    QObject::connect(worker, &SendWorker::finished, worker, &SendWorker::deleteLater);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

}
