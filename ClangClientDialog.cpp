#include <ranges>

#include <QItemSelectionModel>
#include <QInputDialog>
#include <QShortcut>
#include <QMessageBox>
#include <QTextCursor>
#include <QLineEdit>
#include <QTableWidgetItem>

#include "ClangClientDialog.hpp"

#include "ui_ClangClientDialog.h"

#include "JsonTreeModel.hpp"

ClangClientDialog::ClangClientDialog(ClangdProject clangdProject, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::ClangClientDialog),
    clangdClient{std::move(clangdProject), this},
    sendReceivedModel{this},
    lastSearchText{},
    startQuerySymbolTimer{this}{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
    ui->sendReceivedListView->setModel(&sendReceivedModel);

    connect(&clangdClient, &ClangdClient::emitLog, this,
            &ClangClientDialog::addToRawLog, Qt::QueuedConnection);
    connect(&clangdClient, &ClangdClient::messageSent, &sendReceivedModel,
            &SendReceiveListModel::addMessageSent, Qt::QueuedConnection);
    connect(&clangdClient, &ClangdClient::messageReceived, &sendReceivedModel,
            &SendReceiveListModel::addMessageReceived, Qt::QueuedConnection);
    connect(ui->sendReceivedListView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &ClangClientDialog::onMessageSelected);
    connect(ui->clientMessageTreeView, &QTreeView::expanded, this, &ClangClientDialog::onColumnExpandedCollapsed);
    connect(ui->clientMessageTreeView, &QTreeView::collapsed, this, &ClangClientDialog::onColumnExpandedCollapsed);
    connect(ui->serverMessageTreeView, &QTreeView::expanded, this, &ClangClientDialog::onColumnExpandedCollapsed);
    connect(ui->serverMessageTreeView, &QTreeView::collapsed, this, &ClangClientDialog::onColumnExpandedCollapsed);

    connect(ui->symbolSearchLineEdit, &QLineEdit::textChanged, this, &ClangClientDialog::onSymbolSearchTextChanged);

    // Create a shortcut for Ctrl+F
    QShortcut* findShortcut = new QShortcut(QKeySequence("Ctrl+F"), ui->rawLogPlainTextEdit);
    connect(findShortcut, &QShortcut::activated, this, &ClangClientDialog::showFindDialog);

    // Create a shortcut for F3 (Find Next)
    QShortcut* findNextShortcut = new QShortcut(QKeySequence("F3"), ui->rawLogPlainTextEdit);
    connect(findNextShortcut, &QShortcut::activated, this, &ClangClientDialog::findNext);

    // Create a shortcut for Shift+F3 (Find Previous)
    QShortcut* findPrevShortcut = new QShortcut(QKeySequence("Shift+F3"), ui->rawLogPlainTextEdit);
    connect(findPrevShortcut, &QShortcut::activated, this, &ClangClientDialog::findPrevious);

    startQuerySymbolTimer.setSingleShot(true);
    connect(&startQuerySymbolTimer, &QTimer::timeout, this, &ClangClientDialog::onStartQuerySymbolTimerExpired);
}

void ClangClientDialog::addToRawLog(QString stringToLog) {
    ui->rawLogPlainTextEdit->appendPlainText(stringToLog + "\n");
}

void ClangClientDialog::onMessageSelected(const QItemSelection &selected,
                                          const QItemSelection &/*deselected*/) {
    const auto curSelectedItem =
        selected.indexes()[0].data(Qt::UserRole).value<SendReceiveElement>();
    //addToRawLog("Item selected:\n" + curSelectedItem.sent.toJson());
    auto replaceModel = [this](QTreeView* view, const QJsonDocument& newModel)
    {
        auto* oldModel = view->model();
        if(newModel.isNull())
        {
            view->setModel(nullptr);
        }
        else
        {
            view->setModel(new JsonTreeModel{newModel, this});
            emit view->expanded(view->model()->index(0, 0));
        }
        if(oldModel) delete oldModel;
    };
    replaceModel(ui->clientMessageTreeView, curSelectedItem.sent);
    replaceModel(ui->serverMessageTreeView, curSelectedItem.received);
}

void ClangClientDialog::onColumnExpandedCollapsed(const QModelIndex &/*index*/)
{
    QTreeView* senderObject = static_cast<QTreeView*>(sender());
    for(auto index = 0; index < senderObject->model()->columnCount(); ++index){
        senderObject->resizeColumnToContents(index);
    }
}

void ClangClientDialog::showFindDialog() {
    // Show an input dialog to get the text to find
    bool ok;
    QString text = QInputDialog::getText(this, tr("Find"),
                                         tr("Find what:"), QLineEdit::Normal,
                                         QString(), &ok);
    if (ok && !text.isEmpty()) {
        lastSearchText = text;
        clearHighlights();
        this->activateWindow();
        ui->rawLogPlainTextEdit->raise();
        highlightAllOccurrences();
        findText(text);
    }
}

void ClangClientDialog::findText(const QString &text) {
    // Move the cursor to the start of the document
    ui->rawLogPlainTextEdit->moveCursor(QTextCursor::Start);

    // Try to find the text
    if (!ui->rawLogPlainTextEdit->find(text)) {
        QMessageBox::information(this, tr("Find"), tr("The text was not found."));
    }
}

void ClangClientDialog::findNext() {
    if (lastSearchText.isEmpty()) {
        // If no search text is available, show the find dialog
        showFindDialog();
        return;
    }

    // Try to find the text from the current cursor position
    if (!ui->rawLogPlainTextEdit->find(lastSearchText)) {
        // If not found, wrap around to the start and try again
        ui->rawLogPlainTextEdit->moveCursor(QTextCursor::Start);
        if (!ui->rawLogPlainTextEdit->find(lastSearchText)) {
            QMessageBox::information(this, tr("Find"), tr("The text was not found."));
        }
    }
}

void ClangClientDialog::findPrevious() {
    if (lastSearchText.isEmpty()) {
        // If no search text is available, show the find dialog
        showFindDialog();
        return;
    }

    // Try to find the text in the reverse direction
    if (!ui->rawLogPlainTextEdit->find(lastSearchText, QTextDocument::FindBackward)) {
        // If not found, wrap around to the end and try again in reverse
        ui->rawLogPlainTextEdit->moveCursor(QTextCursor::End);
        if (!ui->rawLogPlainTextEdit->find(lastSearchText, QTextDocument::FindBackward)) {
            QMessageBox::information(this, tr("Find"), tr("The text was not found."));
        }
    }
}

auto enumerate(const auto& data) {
    return data | std::views::transform([i = 0](const auto& value) mutable {
               return std::make_pair(i++, value);
           });
}

void ClangClientDialog::onStartQuerySymbolTimerExpired()
{
    const QString& text = ui->symbolSearchLineEdit->text();
    if(text.isEmpty())
    {
        return;
    }
    static QStringList headers({"Name", "Kind", "File URI", "Start", "End", "Score"});
    const std::vector<SymbolInfo> v = clangdClient.querySymbol(text);
    ui->symbolTableWidget->setColumnCount(headers.size());
    ui->symbolTableWidget->setRowCount(v.size());
    ui->symbolTableWidget->setHorizontalHeaderLabels(headers);
    for(const auto& [i, symbol] : enumerate(v))
    {
        ui->symbolTableWidget->setItem(i, 0, new QTableWidgetItem(symbol.name));
        ui->symbolTableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(static_cast<int>(symbol.kind))));
        ui->symbolTableWidget->setItem(i, 2, new QTableWidgetItem(symbol.fileUri));
        ui->symbolTableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(symbol.startPos.first) + ":" + QString::number(symbol.startPos.second)));
        ui->symbolTableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(symbol.endPos.first) + ":" + QString::number(symbol.endPos.second)));
        ui->symbolTableWidget->setItem(i, 5, new QTableWidgetItem(QString::number(symbol.score)));
    }
    ui->symbolTableWidget->resizeColumnsToContents();
}

void ClangClientDialog::onSymbolSearchTextChanged(const QString &/*text*/)
{
    ui->symbolTableWidget->clear();
    ui->symbolTableWidget->setRowCount(0);
    ui->symbolTableWidget->setColumnCount(0);
    startQuerySymbolTimer.start(200);
}

void ClangClientDialog::clearHighlights() {
#if 0
    // Create a QTextCursor for the entire document
    QTextCursor cursor(ui->rawLogPlainTextEdit->document());
    cursor.select(QTextCursor::Document);
    QTextCharFormat clearFormat;
    cursor.setCharFormat(clearFormat);  // Clear all highlights
#endif
}

void ClangClientDialog::highlightAllOccurrences() {
#if 0
    QTextCursor cursor(ui->rawLogPlainTextEdit->document());
    QTextCharFormat highlightFormat;
    highlightFormat.setBackground(Qt::yellow); // Set the highlight color

    // Search for the text and highlight it
    cursor.setPosition(0);
    while (cursor.find(lastSearchText)) {
        cursor.mergeCharFormat(highlightFormat);
    }
#endif
}

ClangClientDialog::~ClangClientDialog() {}
