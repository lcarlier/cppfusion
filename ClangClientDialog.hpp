#pragma once

#include <memory>

#include <QDialog>
#include <QString>
#include <QJsonDocument>
#include <QModelIndex>
#include <QItemSelection>
#include <QTextCharFormat>
#include <QTimer>
#include <QPoint>

#include "ClangdClient.hpp"
#include "SendReceiveListModel.hpp"

namespace Ui {
class ClangClientDialog;
}

class ClangClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClangClientDialog(ClangdClient& clangdClient, const ClangdProject& clangdProject, QWidget *parent = nullptr);
    ~ClangClientDialog();
public slots:
    void addToRawLog(QString stringToLog);

private:
    std::unique_ptr<Ui::ClangClientDialog> ui;
    ClangdClient& clangdClient;
    SendReceiveListModel sendReceivedModel;
    QString lastSearchText;
    QTimer startQuerySymbolTimer;
    void findText(const QString &text);
    void findNext();
    void findPrevious();
    void clearHighlights();
    void highlightAllOccurrences();

private slots:
    void onMessageSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void onColumnExpandedCollapsed(const QModelIndex &index);
    void showFindDialog();
    void onStartQuerySymbolTimerExpired();
    void onSymbolSearchTextChanged(const QString &text);
    void onOpenCloseRightClick(const QPoint &pos);
};
