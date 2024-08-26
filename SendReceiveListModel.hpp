#ifndef SENDRECEIVELISTMODEL_H
#define SENDRECEIVELISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QJsonDocument>

struct SendReceiveElement
{
    QJsonDocument sent;
    QJsonDocument received;
};

Q_DECLARE_METATYPE(SendReceiveElement);

class SendReceiveListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SendReceiveListModel(QObject *parent = nullptr);
    SendReceiveListModel(const SendReceiveListModel&) = delete;
    SendReceiveListModel(SendReceiveListModel&&) = delete;
    SendReceiveListModel& operator=(const SendReceiveListModel&) = delete;
    SendReceiveListModel& operator=(SendReceiveListModel&&) = delete;

    // Header:
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void addMessageSent(QJsonDocument messageSent);
    void addMessageReceived(QJsonDocument messageReceived);

private:
    QList<SendReceiveElement> elements;

    void appendElement(const SendReceiveElement&& element);
};

#endif // SENDRECEIVELISTMODEL_H
