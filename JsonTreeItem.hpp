#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>
#include <QVector>

#include <vector>
#include <memory>

class JsonTreeItem
{
public:
    explicit JsonTreeItem(const QString &key, const QJsonValue &value, JsonTreeItem *parent = nullptr)
        : m_key(key), m_value(value), m_parent(parent) {}

    /*~JsonTreeItem() {
        qDeleteAll(m_children);
    }*/

    void appendChild(std::unique_ptr<JsonTreeItem> child) {
        m_children.push_back(std::move(child));
    }

    JsonTreeItem *child(int row) const {
        return m_children[row].get();
    }

    int childCount() const {
        return m_children.size();
    }

    int row() const {
        if (m_parent)
        {
            //return m_parent->m_children.(const_cast<JsonTreeItem*>(this));
            for(uint32_t i = 0; i < m_parent->m_children.size(); ++i)
            {
                if(m_parent->m_children.at(i).get() == this)
                {
                    return i;
                }
            }
        }
        return 0;
    }

    JsonTreeItem *parentItem() const {
        return m_parent;
    }

    QString key() const {
        return m_key;
    }

    QJsonValue value() const {
        return m_value;
    }

private:
    QString m_key;
    QJsonValue m_value;
    JsonTreeItem *m_parent;
    std::vector<std::unique_ptr<JsonTreeItem>> m_children;
};
