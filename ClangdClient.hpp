#ifndef CLANGDCLIENT_H
#define CLANGDCLIENT_H

#include <optional>
#include <functional>
#include <vector>
#include <unordered_map>

#include <QObject>
#include <QProcess>
#include <QJsonDocument>
#include <QString>
#include <QThread>
#include <QFileInfo>

struct ClangdProject {
    QString projectRoot;
    QString compileCommandJson;
    QString clangdPath;
};

struct SymbolInfo {
    using Position = std::pair<int, int>;
    enum class Kind {
        File = 1,
        Module = 2,
        Namespace = 3,
        Package = 4,
        Class = 5,
        Method = 6,
        Property = 7,
        Field = 8,
        Constructor = 9,
        Enum = 10,
        Interface = 11,
        Function = 12,
        Variable = 13,
        Constant = 14,
        String = 15,
        Number = 16,
        Boolean = 17,
        Array = 18,
        Object = 19,
        Key = 20,
        Null = 21,
        EnumMember = 22,
        Struct = 23,
        Event = 24,
        Operator = 25,
        TypeParameter = 26,
    };
    QString name;
    Kind kind;
    QString fileUri;
    Position startPos;
    Position endPos;
    double score;
};

using Cb = std::function<void(const QJsonDocument&)>;
using OptionalCb = std::optional<Cb>;

namespace cppfusion::priv {
// I don't like this static variable. Need to find a better solution.
static const QString emptyId{"emptyId"};

static QString getId(const QJsonDocument& jsonDoc)
{
    QString id{emptyId};
    const auto& jsonId = jsonDoc["id"];
    if(jsonId.isDouble())
    {
        id = QString::number(jsonId.toDouble());
    }
    else if(jsonId.isString())
    {
        id = jsonId.toString();
    }
    return id;
}

class ClangdWorker : public QObject {
    Q_OBJECT

public:
    ClangdWorker(const ClangdProject& clangdProject, QObject *parent = nullptr)
        : QObject(parent), clangdProject{clangdProject}, clangd(this), byteToRead{0}, totalOut{}, curCallBack{}
    {
    }
    ~ClangdWorker()
    {
        if (clangd.state() == QProcess::Running)
        {
            clangd.terminate();
            clangd.waitForFinished();
        }
    }


public slots:
    // Slot to write data to the QProcess standard input
    void writeDataToProcess(const QJsonDocument jsonDoc, OptionalCb cb) {
        if (clangd.state() == QProcess::Running) {
            QByteArray data = jsonDoc.toJson(QJsonDocument::Indented);
            QString finalMessage = QString("Content-Length: %1\n\n%2")
                                       .arg(data.size())
                                       .arg(QString(data));
            QString threadIdStr = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
            emit emitLog(QString{"TID: "} + threadIdStr + QString{" sending\n" + finalMessage});
            QString id = getId(jsonDoc);
            if(cb.has_value() && id != emptyId)
            {
                curCallBack[id] = *cb;
            }
            clangd.write(finalMessage.toUtf8());
        }
    }
    void startClangd()
    {
        // Connect process signals to our custom slots
        connect(&clangd, &QProcess::readyReadStandardOutput, this, &ClangdWorker::handleReadyReadStandardOutput);
        connect(&clangd, &QProcess::readyReadStandardError, this, &ClangdWorker::handleReadyReadStandardError);

        clangd.setProgram(clangdProject.clangdPath);
        QFileInfo compileCommands{clangdProject.compileCommandJson};
        clangd.setArguments({"--offset-encoding=utf-8", "--compile-commands-dir=" + compileCommands.absolutePath(), "--log=verbose", "--background-index"});
        clangd.start();
        clangd.waitForStarted();

        emit clangdStarted();
    }

signals:
    // Signal to emit when QProcess has output
    void clangdStarted();
    void processOutput(const QString &output);
    void emitLog(QString stringToLog);
    void messageReceived(QJsonDocument document);

private slots:
    // Slot to handle standard output
    void handleReadyReadStandardOutput() {
        clangd.setReadChannel(QProcess::StandardOutput);
        while(clangd.canReadLine() || byteToRead != 0)
        {
            if(byteToRead == 0)
            {
                QString firstLine = clangd.readLine();
                QString threadIdStr = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
                emit emitLog(QString{"TID: "} + threadIdStr + QString(" received") + "\n" + firstLine);
                qDebug() << "first line received: " << firstLine;
                static const QString CONTENT_LENGTH_REGEX_STR =
                    R"(Content-Length: (\d+).*)";
                static const QRegularExpression CONTENT_LENGTH_REGEX(
                    CONTENT_LENGTH_REGEX_STR);
                QRegularExpressionMatch match = CONTENT_LENGTH_REGEX.match(firstLine);
                if (match.hasMatch())
                {
                    byteToRead = match.captured(1).toLongLong();
                    // skip empty line
                    clangd.readLine();
                }
                else
                {
                    emit emitLog(QString{"Wrong receive first line\n"} + firstLine);
                    continue;
                }
            }

            {
                const QString curOut = clangd.read(byteToRead);
                qDebug() << "Reading data " << byteToRead << " bytes\n";
                const auto nbByteRead = curOut.size();
                if(nbByteRead == 0)
                {
                    qDebug() << "No more bytes to read. Returning and waiting for future signals...";
                    emit emitLog("No more bytes to read. Returning and waiting for future signals...");
                    return;
                }
                byteToRead -= nbByteRead;
                totalOut += curOut;
                if(byteToRead > 0)
                {
                    qDebug() << "Partial read of " + QString::number(nbByteRead);
                    emit emitLog("Partial read of " + QString::number(nbByteRead));
                }
            }

            if(byteToRead == 0)
            {
                QJsonDocument jsonDocument = QJsonDocument::fromJson(totalOut.toUtf8());
                emit messageReceived(jsonDocument);
                emit emitLog(jsonDocument.toJson(QJsonDocument::Indented));
                const QString id = getId(jsonDocument);
                if(auto idx = curCallBack.find(id); idx != curCallBack.end())
                {
                    (idx->second)(jsonDocument);
                    curCallBack.erase(idx);
                }
                totalOut.clear();
            }
        }
    }

    // Slot to handle standard error
    void handleReadyReadStandardError() {
        clangd.setReadChannel(QProcess::StandardError);
        QByteArray output = clangd.readAllStandardError();
        QString error {"Error: "};
        error += QString{output};
        qDebug() << "from stdErr:\n" << output;
        emit emitLog(error);
    }

private:
    const ClangdProject& clangdProject;
    QProcess clangd;
    qint64 byteToRead{0};
    QString totalOut{};
    std::unordered_map<QString, Cb> curCallBack;
};
} // namespace cppfusion::priv

class ClangdClient : public QObject
{
    Q_OBJECT
public:
    explicit ClangdClient(ClangdProject clangdProject, QObject *parent = nullptr);
    ~ClangdClient()
    {
        if(clangdThread.isRunning())
        {
            clangdThread.exit();
            clangdThread.wait();
        }
    }
    void initServer();
    void addFileToDatabse(QString path);
    void openFile(const QString& path);
    void closeFile(const QString& path);
    std::vector<SymbolInfo> querySymbol(QString symbol, double limit = 10000);

private:
    void sendData(const QJsonDocument&, bool useId = true, OptionalCb callback = std::nullopt);

    QJsonDocument getFinalMessage(const QJsonDocument&, bool useId);

    ClangdProject clangdProject;
    QThread clangdThread;
    cppfusion::priv::ClangdWorker clangdWorker;


private slots:
    void clangdStarted();
    void processMessageReceived(QJsonDocument document);
    void forwardEmitLog(QString stringToLog);
signals:
    void startClangd();
    void commandSent(const QJsonDocument message, OptionalCb);
    void emitLog(QString stringToLog);
    void messageSent(QJsonDocument document);
    void messageReceived(QJsonDocument document);
};

#endif // CLANGDCLIENT_H
