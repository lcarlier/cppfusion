#include "SendReceiveListModel.hpp"

#include <QJsonObject>

SendReceiveListModel::SendReceiveListModel(QObject *parent)
    : QAbstractListModel(parent) {}

QVariant SendReceiveListModel::headerData(int /*section*/,
                                          Qt::Orientation /*orientation*/,
                                          int /*role*/) const {
    return QVariant{"Messages"};
}

int SendReceiveListModel::rowCount(const QModelIndex &parent) const {
    // For list models only the root node (an invalid parent) should return the
    // list's size. For all other (valid) parents, rowCount() should return 0 so
    // that it does not become a tree model.
    if (parent.isValid()) return 0;

    return elements.count();
}

QVariant SendReceiveListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    const auto curRow = index.row();
    if (curRow >= elements.count()) {
        return QVariant{};
    }

    if (role == Qt::DisplayRole) {
        const auto &curElement = elements.at(curRow);
        QString valToShow{};
        if (curElement.sent.isObject()) {
            const auto &curSendMsg = curElement.sent.object();
            valToShow = curSendMsg["method"].toString();
        }
        if (curElement.received.isObject()) {
            const auto &curReceivedMsg = curElement.received.object();
            valToShow += "\n" + curReceivedMsg["method"].toString("no method");
        }
        return QVariant{valToShow};
    } else if (role == Qt::UserRole) {
        QVariant rv{};
        rv.setValue(elements.at(curRow));
        return rv;
    }
    return QVariant();
}

void SendReceiveListModel::addMessageSent(QJsonDocument messageSend) {
    if(!messageSend.object().contains("id"))
    {
        appendElement(SendReceiveElement{messageSend, {}});
    }
    else
    {
        const auto maxRow = elements.count();
        for (auto idx = maxRow - 1; idx >= 0; --idx) {
            auto &curRow = elements[idx];
            if (curRow.received.object()["id"] == messageSend.object()["id"]) {
                curRow.sent = messageSend;
                const auto changedIndex = index(idx, 0);
                emit dataChanged(changedIndex, changedIndex);
                return;
            }
        }
        // Fallback. Append the message sent
        appendElement(SendReceiveElement{messageSend, {}});
    }
}

void SendReceiveListModel::addMessageReceived(QJsonDocument messageReceived) {
    const auto maxRow = elements.count();
    if (maxRow == 0) {
        return;
    }
    if(!messageReceived.object().contains("id"))
    {
        appendElement(SendReceiveElement{{}, messageReceived});
    }
    else
    {
        for (auto idx = maxRow - 1; idx >= 0; --idx) {
            auto &curRow = elements[idx];
            if (curRow.sent.object()["id"] == messageReceived.object()["id"]) {
                curRow.received = messageReceived;
                const auto changedIndex = index(idx, 0);
                emit dataChanged(changedIndex, changedIndex);
                return;
            }
        }
        // Fallback. Append the message received
        appendElement(SendReceiveElement{{}, messageReceived});
    }
}

void SendReceiveListModel::appendElement(const SendReceiveElement&& element)
{
    const auto curNbElem = elements.count();
    beginInsertRows(QModelIndex{}, curNbElem, curNbElem);
    elements.append(std::move(element));
    endInsertRows();
}
