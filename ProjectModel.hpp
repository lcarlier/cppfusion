#pragma once

#include <memory>

#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QVariant>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QList>
#include <QFuture>
#include <QtConcurrent>

#include "ClangdClient.hpp"
#include "QFileRAII.hpp"
#include "JsonHelper.hpp"

class TreeItem
{
public:
    explicit TreeItem(QVariantList data, TreeItem *parentItem = nullptr)
        : m_itemData(std::move(data)), m_parentItem(parentItem)
    {}

    TreeItem* appendChild(std::unique_ptr<TreeItem> &&child)
    {
        m_childItems.push_back(std::move(child));
        return m_childItems.back().get();
    }

    TreeItem *child(int row)
    {
        return row >= 0 && row < childCount() ? m_childItems.at(row).get() : nullptr;
    }
    int childCount() const
    {
        return int(m_childItems.size());
    }
    int columnCount() const
    {
        return int(m_itemData.count());
    }
    QVariant data(int column) const
    {
        return m_itemData.value(column);
    }
    int row() const
    {
        if (m_parentItem == nullptr)
            return 0;
        const auto it = std::find_if(m_parentItem->m_childItems.cbegin(), m_parentItem->m_childItems.cend(),
                                     [this](const std::unique_ptr<TreeItem> &treeItem) {
                                         return treeItem.get() == this;
                                     });

        if (it != m_parentItem->m_childItems.cend())
            return std::distance(m_parentItem->m_childItems.cbegin(), it);
        Q_ASSERT(false); // should not happen
        return -1;
    }
    TreeItem *parentItem()
    {
        return m_parentItem;
    }

private:
    std::vector<std::unique_ptr<TreeItem>> m_childItems;
    QVariantList m_itemData;
    TreeItem *m_parentItem;
};

class ProjectModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(ProjectModel)

    explicit ProjectModel(const ClangdProject& clangdProject, QObject *parent = nullptr): QAbstractItemModel(parent)
        , rootItem(std::make_unique<TreeItem>(QVariantList{tr("File")}))
    {
        QFileInfo topProjectQdir{clangdProject.projectRoot};
        TreeItem* projectRoot = rootItem->appendChild(std::make_unique<TreeItem>(QVariantList{QVariant{topProjectQdir.fileName()}}, rootItem.get()));
        TreeItem* sourceFileItem = projectRoot->appendChild(std::make_unique<TreeItem>(QVariantList{QVariant{"Source files"}}, projectRoot));

        QStringList validFiles;
        {
            QFileRAII compileCommands{clangdProject.compileCommandJson};
            const QJsonDocument compileCommandsJson = QJsonDocument::fromJson(compileCommands.readAll().toUtf8());
            const QJsonArray& jsonArray = compileCommandsJson.array();
            QList<QFuture<void>> futures;
            QMutex mutex;
            for(const auto& elem: jsonArray)
            {
                QFuture<void> future = QtConcurrent::run([&](const QJsonObject curObject)
                {
                    QStringList allFiles = getIncludedHeaderFiles(curObject, clangdProject.projectRoot);
                    QString fullPath = getFullPathFromCompileCommandElement(curObject);
                    QMutexLocker locker(&mutex);
                    validFiles.append(fullPath);
                    validFiles.append(allFiles);
                }, elem.toObject());
                futures.append(future);
            }
            for(auto& future : futures)
            {
                future.waitForFinished();
            }
        }

        populateSourceFile(sourceFileItem, QDir{clangdProject.projectRoot}, validFiles);
    }

    void populateSourceFile(TreeItem* parent, QDir curDir, const QStringList& validFiles)
    {
        QFileInfoList fileList = curDir.entryInfoList(QDir::Filter::Dirs | QDir::Filter::Files | QDir::Filter::NoDotAndDotDot, QDir::SortFlag::Name | QDir::SortFlag::DirsFirst);
        for(const QFileInfo& fileInfo : fileList)
        {
            QString filePath = fileInfo.absoluteFilePath();
            for(const auto& f : validFiles)
            {
                if(f.contains(filePath))
                {

                    QVariantList varList{QVariant{fileInfo.fileName()}};
                    if(fileInfo.isFile())
                    {
                        varList.append(QVariant{filePath});
                    }
                    TreeItem* thisParent = parent->appendChild(std::make_unique<TreeItem>(std::move(varList), parent));
                    if (fileInfo.isDir()) {
                        populateSourceFile(thisParent, QDir{filePath}, validFiles);
                    }
                    break;
                }
            }
        }
    }

    ~ProjectModel() override = default;

    QString filePath(const QModelIndex &index) const
    {
        const auto *item = static_cast<const TreeItem*>(index.internalPointer());
        return item->data(1).toString();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};

        const auto *item = static_cast<const TreeItem*>(index.internalPointer());
        return item->data(index.column());
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if(index.isValid())
        {
            return QAbstractItemModel::flags(index);
        }
        return Qt::ItemFlags(Qt::NoItemFlags);
    }
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override
    {
        return orientation == Qt::Horizontal && role == Qt::DisplayRole
                   ? rootItem->data(section) : QVariant{};
    }
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = {}) const override
    {

        if (!hasIndex(row, column, parent))
            return {};
        TreeItem *parentItem = parent.isValid()
                                   ? static_cast<TreeItem*>(parent.internalPointer())
                                   : rootItem.get();

        if (auto *childItem = parentItem->child(row))
            return createIndex(row, column, childItem);
        return {};
    }
    QModelIndex parent(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return {};

        auto *childItem = static_cast<TreeItem*>(index.internalPointer());
        TreeItem *parentItem = childItem->parentItem();

        return parentItem != rootItem.get()
                   ? createIndex(parentItem->row(), 0, parentItem) : QModelIndex{};
    }
    int rowCount(const QModelIndex &parent = {}) const override
    {
        if (parent.column() > 0)
            return 0;

        const TreeItem *parentItem = parent.isValid()
                                         ? static_cast<const TreeItem*>(parent.internalPointer())
                                         : rootItem.get();
        return parentItem->childCount();
    }
    int columnCount(const QModelIndex &/*parent*/ = {}) const override
    {
        return 1;
    }


private:
    std::unique_ptr<TreeItem> rootItem;
};
