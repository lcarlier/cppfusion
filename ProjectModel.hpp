#pragma once

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QVariant>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonArray>

#include "ClangdClient.hpp"
#include "QFileRAII.hpp"
#include "JsonHelper.hpp"

class ExtensionFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    ExtensionFilterProxyModel(const ClangdProject& clangdProject, QObject *parent = nullptr)
        : QSortFilterProxyModel(parent), validFiles{}
    {
        QFileRAII compileCommands{clangdProject.compileCommandJson};
        const QJsonDocument compileCommandsJson = QJsonDocument::fromJson(compileCommands.readAll().toUtf8());
        const QJsonArray& jsonArray = compileCommandsJson.array();
        for(const auto& elem: jsonArray)
        {
            const QJsonObject& curObject = elem.toObject();
            validFiles.append(getFullPathFromCompileCommandElement(curObject));
        }
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!index.isValid())
            return false;

        QFileSystemModel *model = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!model)
            return false;

        QFileInfo fileInfo = model->fileInfo(index);
        QString filePath = fileInfo.absoluteFilePath();

        for(const auto& f : validFiles)
        {
            if(f.contains(filePath))
            {
                return true;
            }
        }

        return false;
    }

private:
    QStringList validFiles;
};

class ProjectModel : public QFileSystemModel
{
    Q_OBJECT


public:
    ProjectModel(const ClangdProject& clangdProject, QObject* parent = nullptr)
        : QFileSystemModel(parent)
    {
        this->setReadOnly(true);
        this->setRootPath(clangdProject.projectRoot);
    }
};
