#include <cstdio>
#include <optional>
#include <functional>
#include <utility>
#include <initializer_list>

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QMessageBox>

#include "ClangdClient.hpp"
#include "QFileRAII.hpp"

ClangdClient::ClangdClient(ClangdProject clangdProject_p, QObject *parent) : QObject{parent}, clangdProject{std::move(clangdProject_p)}, clangdThread{}, clangdWorker{clangdProject}, curId{0}
{
    clangdWorker.moveToThread(&clangdThread);
    connect(this, &ClangdClient::startClangd, &clangdWorker, &cppfusion::priv::ClangdWorker::startClangd);
    connect(this, &ClangdClient::commandSent, &clangdWorker, &cppfusion::priv::ClangdWorker::writeDataToProcess, Qt::QueuedConnection);
    connect(&clangdWorker, &cppfusion::priv::ClangdWorker::emitLog, this, &ClangdClient::forwardEmitLog, Qt::QueuedConnection);
    connect(&clangdWorker, &cppfusion::priv::ClangdWorker::messageReceived, this, &ClangdClient::forwardMessageReceived, Qt::QueuedConnection);
    connect(&clangdWorker, &cppfusion::priv::ClangdWorker::clangdStarted, this, &ClangdClient::clangdStarted, Qt::QueuedConnection);
    clangdThread.setObjectName("ClangThread");
    clangdThread.start();
    emit startClangd();
}
using JsonKeyVal = std::pair<QString, QJsonValue>;

static QJsonObject getMessage(QString method, std::initializer_list<JsonKeyVal> params)
{
    static JsonKeyVal jsonRpc{"jsonrpc", "2.0"};
    return QJsonObject{jsonRpc,
                       {"method", method},
                       {"params", QJsonObject{params}}};
}

void ClangdClient::initServer()
{
    QJsonObject textDocumentCapabilities{
                                         {"synchronization", QJsonObject{{"dynamicRegistration", true},
                                                                         {"willSave", true},
                                                                         {"willSaveWaitUntil", true},
                                                                         {"didSave", true}}},
                                         {"completion",
                                          QJsonObject{
                                                      {"dynamicRegistration", true},
                                                      {"completionItem", QJsonObject{{"snippetSupport", true},
                                                                                     {"commitCharactersSupport", true},
                                                                                     {"documentationFormat",
                                                                                      QJsonArray{"markdown", "plaintext"}},
                                                                                     {"deprecatedSupport", true},
                                                                                     {"preselectSupport", true}}},
                                                      {"contextSupport", true}}},
                                         {"hover",
                                          QJsonObject{{"dynamicRegistration", true},
                                                      {"contentFormat", QJsonArray{"markdown", "plaintext"}}}},
                                         {"signatureHelp",
                                          QJsonObject{{"dynamicRegistration", true},
                                                      {"signatureInformation",
                                                       QJsonObject{{"documentationFormat",
                                                                    QJsonArray{"markdown", "plaintext"}},
                                                                   {"parameterInformation",
                                                                    QJsonObject{{"labelOffsetSupport", true}}}}}}},
                                         {"references", QJsonObject{{"dynamicRegistration", true}}},
                                         {"documentSymbol",
                                          QJsonObject{
                                                      {"dynamicRegistration", true},
                                                      {"symbolKind",
                                                       QJsonObject{{"valueSet",
                                                                    QJsonArray{1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
                                                                               12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}}}}}},
                                         {"workspaceSymbol", QJsonObject{{"dynamicRegistration", true}}},
                                         {"codeAction",
                                          QJsonObject{
                                                      {"dynamicRegistration", true},
                                                      {"codeActionLiteralSupport",
                                                       QJsonObject{
                                                                   {"codeActionKind",
                                                                    QJsonObject{{"valueSet",
                                                                                 QJsonArray{"empty", "quickfix", "refactor",
                                                                                            "refactor.extract", "refactor.inline",
                                                                                            "refactor.rewrite", "source",
                                                                                            "source.organizeImports"}}}}}}}},
                                         {"codeLens", QJsonObject{{"dynamicRegistration", true}}},
                                         {"documentLink", QJsonObject{{"dynamicRegistration", true}}},
                                         {"colorProvider", QJsonObject{{"dynamicRegistration", true}}},
                                         {"rename", QJsonObject{{"dynamicRegistration", true},
                                                                {"prepareSupportDefaultBehavior", true}}}};

    QJsonObject workspaceCapabilities{
                                      {"applyEdit", true},
                                      {"workspaceEdit", QJsonObject{{"documentChanges", true}}},
                                      {"didChangeConfiguration", QJsonObject{{"dynamicRegistration", true}}},
                                      {"didChangeWatchedFiles", QJsonObject{{"dynamicRegistration", true}}},
                                      {"symbol", QJsonObject{{"dynamicRegistration", true}}},
                                      {"executeCommand", QJsonObject{{"dynamicRegistration", true}}}};

    QJsonObject windowCapabilities{
                                   {"showMessage", QJsonObject{{"dynamicRegistration", true}}},
                                   {"showMessageRequest", QJsonObject{{"dynamicRegistration", true}}},
                                   {"logMessage", QJsonObject{{"dynamicRegistration", true}}}};

    QJsonObject capabilities{{"textDocument", textDocumentCapabilities},
                             {"workspace", workspaceCapabilities},
                             {"window", windowCapabilities}};

    /*QJsonArray workspaceFolders{
                                QJsonObject{{"uri", "file:///home/lcarlier/Projects/cpp/EasyMock"},
                                            {"name", "EasyMock"}}};*/

    QJsonObject params{{"processId", QCoreApplication::applicationPid()},
                       {"rootUri", "file://" + clangdProject.projectRoot},
                       {"capabilities", capabilities},
                       {"initializationOptions", QJsonObject()},
                       {"trace", "off"}/*,
                       {"workspaceFolders", workspaceFolders}*/};

    QJsonObject init_message_json{{"jsonrpc", "2.0"},
                                  {"id", 0},
                                  {"method", "initialize"},
                                  {"params", params}};

    // Create QJsonDocument
    QJsonDocument init_message_doc(init_message_json);

    QMutex mutex;
    QWaitCondition condition;
    sendData(init_message_doc, true, [this, &mutex, &condition](const QJsonDocument&)
             {
                 QMutexLocker locker(&mutex);
                 QJsonObject initialized_message_json{{"jsonrpc", "2.0"},
                                                      {"method", "initialized"},
                                                      {"params", QJsonObject{}}};

                 // Create QJsonDocument
                 QJsonDocument initialized_message_doc(initialized_message_json);
                 sendData(initialized_message_doc, false);
                 condition.wakeAll();
             });
    QMutexLocker locker(&mutex);
    condition.wait(&mutex);
}

void ClangdClient::addFileToDatabse(QString path)
{

    QString uri = "file://" + path;
    {
        QFileRAII file{path};
        QString content = file.readAll();
        QJsonObject message = getMessage("textDocument/didOpen",
                                         {{"textDocument",
                                             QJsonObject{
                                                 {"languageId", "cpp"},
                                                 {"text", content},
                                                 {"uri", uri},
                                                 {"version", 0}
                                             }
                                         }});
        sendData(QJsonDocument{message}, false);
    }
    {
        QJsonObject message = getMessage("textDocument/didClose",
                                         {{"textDocument",
                                             QJsonObject{
                                                 {"uri", uri}
                                             }
                                         }});
        sendData(QJsonDocument{message}, false);
    }

}

static SymbolInfo::Position getPosition(const QJsonObject& obj)
{
    return std::make_pair(obj["line"].toInt(), obj["character"].toInt());
}

std::vector<SymbolInfo> ClangdClient::querySymbol(QString symbol, double limit)
{
    QMutex mutex;
    QWaitCondition condition;
    QJsonObject message = getMessage("workspace/symbol",
                                     {{"limit", limit},
                                      {"query", symbol}});
    std::vector<SymbolInfo> rv;
    sendData(QJsonDocument{message}, true, [&rv, &mutex, &condition](const QJsonDocument& answer)
             {
                 QMutexLocker locker(&mutex);
                 const auto& results = answer["result"].toArray();
                 rv.reserve(results.count());
                 for(const auto& result: results)
                 {
                     const auto& resultObj = result.toObject();
                     const auto& name = resultObj["name"].toString();
                     const auto& kind = resultObj["kind"].toInt();
                     const auto& location = resultObj["location"].toObject();
                     const auto& range = location["range"].toObject();
                     const auto& score = resultObj["score"].toDouble();

                     rv.emplace_back(name, SymbolInfo::Kind{kind}, location["uri"].toString(), getPosition(range["start"].toObject()), getPosition(range["end"].toObject()), score);
                 }
                 condition.wakeAll();
             });
    QMutexLocker locker(&mutex);
    condition.wait(&mutex);

    return rv;
}

void ClangdClient::clangdStarted()
{
    initServer();
    {
        QFileRAII compileCommands{clangdProject.compileCommandJson};
        QJsonDocument compileCommandsJson = QJsonDocument::fromJson(compileCommands.readAll().toUtf8());
        QString firstFile = compileCommandsJson[0]["file"].toString();
        addFileToDatabse(firstFile);
    }
}

void ClangdClient::forwardMessageReceived(QJsonDocument document)
{
    emit messageReceived(document);
}
void ClangdClient::forwardEmitLog(QString stringToLog)
{
    emit emitLog(stringToLog);
}

void ClangdClient::sendData(const QJsonDocument &jsonData, bool useId, std::optional<std::function<void(const QJsonDocument&)>> callback) {
    if (!jsonData.isEmpty()) {
        QJsonDocument finalMessage = getFinalMessage(jsonData, useId);

        emit commandSent(finalMessage, callback);
    }
}

QJsonDocument ClangdClient::getFinalMessage(const QJsonDocument& jsonData, bool useId)
{
    QJsonObject jsonObject = jsonData.object();
    if (useId) {
        jsonObject["id"] = curId;
        curId++;
    }
    QJsonDocument doc(jsonObject);

    // responseArea->appendPlainText("sending\n" + finalMessage);
    emit messageSent(doc);
    return doc;
}
