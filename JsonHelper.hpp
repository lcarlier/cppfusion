#pragma once

#include <QString>
#include <QJsonObject>
#include <QDir>

static QString getFullPathFromCompileCommandElement(const QJsonObject& object)
{
    const QDir filePath{object["file"].toString()};
    QString fullPath{QDir::toNativeSeparators(filePath.path())};
    if(filePath.isRelative())
    {
        fullPath = object["directory"].toString() + "/" + filePath.path();
    }
    return fullPath;
}
