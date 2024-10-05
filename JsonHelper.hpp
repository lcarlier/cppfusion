#pragma once

#include <unordered_map>
#include <sstream>
#include <string>
#include <tuple>

#include <QString>
#include <QJsonObject>
#include <QDir>
#include <QStringList>
#include <QProcess>

inline QString getFullPathFromCompileCommandElement(const QJsonObject& object)
{
    const QDir filePath{object["file"].toString()};
    QString fullPath{QDir::toNativeSeparators(filePath.path())};
    if(filePath.isRelative())
    {
        fullPath = object["directory"].toString() + "/" + filePath.path();
    }
    return fullPath;
}

inline std::tuple<QString, QStringList> getCommandLineWithoutO(const QString& command)
{
    std::string program;
    QStringList arguments{"-M"};
    std::istringstream iss{command.toStdString()};
    std::string value{};
    iss >> program;
    while (iss >> value) {
        if (value.rfind("-o", 0) == 0) {  // Check if it's an argument (starts with - or --)
            continue;
        }
        arguments.append(QString::fromStdString(value));
    }
    return std::make_tuple(QString::fromStdString(program), arguments);
}

inline QStringList getIncludedHeaderFiles(const QJsonObject& object, const QString& projectRoot)
{
    const QString cwd{object["directory"].toString()};
    auto [program, arguments] = getCommandLineWithoutO(QString{object["command"].toString()});
    QProcess process{};
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();
    process.setWorkingDirectory(cwd);
    if(!process.waitForFinished()) {
        return {};
    }
    QString output{process.readAllStandardOutput()};
    QStringList files = output.split(" \\\n");
    files.pop_front();
    QStringList rv;
    for(const auto& file : files)
    {
        QString cleanPath = QDir::cleanPath(file);
        if(cleanPath.contains(projectRoot))
        {
            rv.append(cleanPath);
        }
    }
    return rv;
}
