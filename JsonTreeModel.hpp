#pragma once

#include <QAbstractItemModel>
#include <QJsonDocument>

#include "JsonTreeItem.hpp"

class JsonTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit JsonTreeModel(const QJsonDocument &document, QObject *parent = nullptr)
        : QAbstractItemModel(parent), m_rootItem(nullptr) {
        m_rootItem = std::make_unique<JsonTreeItem>("Root", QJsonValue());
        setupModelData(document.object(), m_rootItem.get());
    }

    int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const override {
        return 3;  // One column for key, one for value
    }

    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid())
            return QVariant();

        JsonTreeItem *item = static_cast<JsonTreeItem*>(index.internalPointer());
        auto itemType = item->value().type();

        if (role == Qt::DisplayRole) {
            if (index.column() == 0)
            {
                return item->key();
            }
            if (index.column() == 1)
            {
                switch(itemType){
                case QJsonValue::Type::Array:
                    case QJsonValue::Type::Object:
                    return QString{"[ "} + QString::number(item->childCount()) + " item(s) ]";
                default:
                    return item->value().toVariant();
                }

            }
            if (index.column() == 2)
            {
                auto typeToStr = [](QJsonValue::Type type) -> QString
                {
                    switch(type)
                    {
                    case QJsonValue::Type::Array:
                        return "Array";
                    case QJsonValue::Type::Bool:
                        return "Bool";
                    case QJsonValue::Type::Double:
                        return "Double";
                    case QJsonValue::Type::Null:
                        return "Null";
                    case QJsonValue::Type::Object:
                        return "Object";
                    case QJsonValue::Type::String:
                        return "String";
                    case QJsonValue::Type::Undefined:
                        return "Undefined";
                    }
                    return "InvalidType";
                };
                return typeToStr(itemType);
            }
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return QAbstractItemModel::flags(index);
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            if (section == 0)
                return QString("Key");
            else if (section == 1)
                return QString("Value");
            else if (section == 2)
                return QString("Type");
        }

        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        JsonTreeItem *parentItem;

        if (!parent.isValid())
            parentItem = m_rootItem.get();
        else
            parentItem = static_cast<JsonTreeItem*>(parent.internalPointer());

        JsonTreeItem *childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        else
            return QModelIndex();
    }

    QModelIndex parent(const QModelIndex &index) const override {
        if (!index.isValid())
            return QModelIndex();

        JsonTreeItem *childItem = static_cast<JsonTreeItem*>(index.internalPointer());
        JsonTreeItem *parentItem = childItem->parentItem();

        if (parentItem == m_rootItem.get())
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        JsonTreeItem *parentItem;
        if (!parent.isValid())
            parentItem = m_rootItem.get();
        else
            parentItem = static_cast<JsonTreeItem*>(parent.internalPointer());

        return parentItem->childCount();
    }

private:
    void setupModelData(const QJsonObject &object, JsonTreeItem *parent) {
        for (auto it = object.begin(); it != object.end(); ++it) {
            auto item = std::make_unique<JsonTreeItem>(it.key(), it.value(), parent);


            if (it.value().isObject()) {
                setupModelData(it.value().toObject(), item.get());
            } else if (it.value().isArray()) {
                setupModelArrayData(it.value().toArray(), item.get());
            }
            parent->appendChild(std::move(item));
        }
    }

    void setupModelArrayData(const QJsonArray &array, JsonTreeItem *parent) {
        for (int i = 0; i < array.size(); ++i) {
            QString key = QString("[%1]").arg(i);
            auto item = std::make_unique<JsonTreeItem>(key, array.at(i), parent);


            if (array.at(i).isObject()) {
                setupModelData(array.at(i).toObject(), item.get());
            } else if (array.at(i).isArray()) {
                setupModelArrayData(array.at(i).toArray(), item.get());
            }
            parent->appendChild(std::move(item));
        }
    }

    std::unique_ptr<JsonTreeItem> m_rootItem;
};
