#pragma once

#include <string_view>

#include <QStandardPaths>
#include <QSettings>
#include <QDir>

class ApplicationSettings
{
public:
    ApplicationSettings(): settings{"lcarlier", "CPP Fusion"} {}
    QDir getLastRootPathSelected()
    {
        const auto homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        return QDir{settings.value(LAST_ROOT_PATH_KEY, homeDir[0]).toString()};
    }
    void setLastRootPathSelected(QDir lastRootPathSelected)
    {
        settings.setValue(LAST_ROOT_PATH_KEY, lastRootPathSelected.absolutePath());
    }
    QDir getDirLastCompileCommandJsonSelected()
    {
        const auto homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
        return QDir{settings.value(LAST_COMPILE_COMMANDS_JSON_KEY, homeDir[0]).toString()};
    }
    void setDirLastCompileCommandJsonSelected(QDir dirLastCompileCommandSelected)
    {
        settings.setValue(LAST_COMPILE_COMMANDS_JSON_KEY, dirLastCompileCommandSelected.absolutePath());
    }
private:
    static constexpr std::string_view LAST_ROOT_PATH_KEY = "project/lastRootPathSelected";
    static constexpr std::string_view LAST_COMPILE_COMMANDS_JSON_KEY = "project/lastCompileCommandsJsonSelected";
    QSettings settings;
};
