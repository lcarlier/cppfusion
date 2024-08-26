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

ClangdClient::ClangdClient(ClangdProject clangdProject_p, QObject *parent) : QObject{parent}, clangdProject{std::move(clangdProject_p)}, clangdThread{}, clangdWorker{clangdProject}
{
    clangdWorker.moveToThread(&clangdThread);
    connect(this, &ClangdClient::startClangd, &clangdWorker, &cppfusion::priv::ClangdWorker::startClangd);
    connect(this, &ClangdClient::commandSent, &clangdWorker, &cppfusion::priv::ClangdWorker::writeDataToProcess, Qt::QueuedConnection);
    connect(&clangdWorker, &cppfusion::priv::ClangdWorker::emitLog, this, &ClangdClient::forwardEmitLog, Qt::QueuedConnection);
    connect(&clangdWorker, &cppfusion::priv::ClangdWorker::messageReceived, this, &ClangdClient::processMessageReceived, Qt::QueuedConnection);
    connect(&clangdWorker, &cppfusion::priv::ClangdWorker::clangdStarted, this, &ClangdClient::clangdStarted, Qt::QueuedConnection);
    clangdThread.setObjectName("ClangThread");
    clangdThread.start();
    emit startClangd();
}
using JsonKeyVal = std::pair<QString, QJsonValue>;

static inline QJsonObject getMessage()
{
    static JsonKeyVal jsonRpc{"jsonrpc", "2.0"};
    return QJsonObject{jsonRpc};
}

static inline QJsonObject getMessage(QString method)
{
    QJsonObject rv = getMessage();
    rv["method"] = method;
    return rv;
}

static inline QJsonObject getMessage(QString method, std::initializer_list<JsonKeyVal> params)
{
    QJsonObject rv = getMessage(method);
    rv["params"] = QJsonObject{params};
    return rv;
}

void ClangdClient::initServer()
{
    QString init_message = R"JSON({
    "jsonrpc": "2.0",
    "method": "initialize",
    "params": {
        "capabilities": {
            "textDocument": {
                "callHierarchy": {
                    "dynamicRegistration": true
                },
                "codeAction": {
                    "codeActionLiteralSupport": {
                        "codeActionKind": {
                            "valueSet": [
                                "*"
                            ]
                        }
                    }
                },
                "completion": {
                    "completionItem": {
                        "commitCharacterSupport": true,
                        "snippetSupport": false
                    },
                    "completionItemKind": {
                        "valueSet": [
                            1,
                            2,
                            3,
                            4,
                            5,
                            6,
                            7,
                            8,
                            9,
                            10,
                            11,
                            12,
                            13,
                            14,
                            15,
                            16,
                            17,
                            18,
                            19,
                            20,
                            21,
                            22,
                            23,
                            24,
                            25
                        ]
                    },
                    "dynamicRegistration": true,
                    "editsNearCursor": true
                },
                "definition": {
                    "dynamicRegistration": true
                },
                "documentSymbol": {
                    "hierarchicalDocumentSymbolSupport": true,
                    "symbolKind": {
                        "valueSet": [
                            1,
                            2,
                            3,
                            4,
                            5,
                            6,
                            7,
                            8,
                            9,
                            10,
                            11,
                            12,
                            13,
                            14,
                            15,
                            16,
                            17,
                            18,
                            19,
                            20,
                            21,
                            22,
                            23,
                            24,
                            25,
                            26
                        ]
                    },
                    "tagSupport": {
                        "valueSet": [
                            1
                        ]
                    }
                },
                "formatting": {
                    "dynamicRegistration": true
                },
                "hover": {
                    "contentFormat": [
                        "markdown",
                        "plaintext"
                    ],
                    "dynamicRegistration": true
                },
                "implementation": {
                    "dynamicRegistration": true
                },
                "inactiveRegionsCapabilities": {
                    "inactiveRegions": true
                },
                "onTypeFormatting": {
                    "dynamicRegistration": true
                },
                "publishDiagnostics": {
                    "categorySupport": true,
                    "codeActionsInline": true
                },
                "rangeFormatting": {
                    "dynamicRegistration": true
                },
                "references": {
                    "dynamicRegistration": true
                },
                "rename": {
                    "dynamicRegistration": true,
                    "prepareSupport": true
                },
                "semanticTokens": {
                    "dynamicRegistration": true,
                    "formats": [
                        "relative"
                    ],
                    "requests": {
                        "full": {
                            "delta": true
                        }
                    },
                    "tokenModifiers": [
                        "declaration",
                        "definition"
                    ],
                    "tokenTypes": [
                        "type",
                        "class",
                        "enumMember",
                        "typeParameter",
                        "parameter",
                        "variable",
                        "function",
                        "macro",
                        "keyword",
                        "comment",
                        "string",
                        "number",
                        "operator"
                    ]
                },
                "signatureHelp": {
                    "dynamicRegistration": true,
                    "signatureInformation": {
                        "activeParameterSupport": true,
                        "documentationFormat": [
                            "markdown",
                            "plaintext"
                        ]
                    }
                },
                "synchronization": {
                    "didSave": true,
                    "dynamicRegistration": true,
                    "willSave": true,
                    "willSaveWaitUntil": false
                },
                "typeDefinition": {
                    "dynamicRegistration": true
                },
                "typeHierarchy": {
                    "dynamicRegistration": true
                }
            },
            "window": {
                "workDoneProgress": true
            },
            "workspace": {
                "applyEdit": true,
                "configuration": true,
                "didChangeConfiguration": {
                    "dynamicRegistration": true
                },
                "executeCommand": {
                    "dynamicRegistration": true
                },
                "semanticTokens": {
                    "refreshSupport": true
                },
                "workspaceEdit": {
                    "documentChanges": true,
                    "resourceOperations": [
                        "create",
                        "rename",
                        "delete"
                    ]
                },
                "workspaceFolders": true
            }
        },
        "clientInfo": {
            "name": "Qt Creator",
            "version": "14.0.1"
        },
        "initializationOptions": {
        },
        "trace": "verbose"
    }
})JSON";
    QJsonObject init_message_obj = QJsonDocument::fromJson(init_message.toUtf8()).object();
    auto params = init_message_obj["params"].toObject();
    params["id"] = QUuid::createUuid().toString();
    params["processId"] = QCoreApplication::applicationPid();
    params["rootUri"] = "file://" + clangdProject.projectRoot;
    params["workspaceFolders"] = QJsonArray{QJsonObject{{"name", "ProjectName"},{"uri","file://" + clangdProject.projectRoot}}};
    init_message_obj["params"] = std::move(params);

    // Create QJsonDocument
    QJsonDocument init_message_doc(std::move(init_message_obj));

    QMutex mutex;
    QWaitCondition condition;
    sendData(init_message_doc, true, [this, &mutex, &condition](const QJsonDocument&)
             {
                 QMutexLocker locker(&mutex);

                 // Create QJsonDocument
                 sendData(QJsonDocument{getMessage("initialized", {})}, false);
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

void ClangdClient::processMessageReceived(QJsonDocument document)
{
    emit messageReceived(document);
    const auto& document_object = document.object();
    const QString& method = document_object["method"].toString();
    if(method == "window/workDoneProgress/create")
    {
        QJsonObject answer = getMessage();
        answer["id"] = document_object["id"].toInt();
        answer["result"] = QJsonValue::Null;
        sendData(QJsonDocument{std::move(answer)}, false, std::nullopt);
    }

}
void ClangdClient::forwardEmitLog(QString stringToLog)
{
    emit emitLog(stringToLog);
}

void ClangdClient::sendData(const QJsonDocument &jsonData, bool useId, std::optional<std::function<void(const QJsonDocument&)>> callback) {
    if (!jsonData.isEmpty()) {
        QJsonDocument finalMessage = getFinalMessage(jsonData, useId);

        emit messageSent(jsonData);
        emit commandSent(finalMessage, callback);
    }
}

QJsonDocument ClangdClient::getFinalMessage(const QJsonDocument& jsonData, bool useId)
{
    QJsonObject jsonObject = jsonData.object();
    if (useId) {
        jsonObject["id"] = QUuid::createUuid().toString();
    }
    QJsonDocument doc(jsonObject);

    // responseArea->appendPlainText("sending\n" + finalMessage);
    return doc;
}
