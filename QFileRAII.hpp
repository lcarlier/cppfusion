#pragma once

#include <QString>
#include <QFile>
#include <QIODevice>
#include <QDebug>
#include <QTextStream>

class QFileRAII {
public:
    QFileRAII(const QString& filePath) : file(filePath) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open file " << filePath << ": " << file.errorString();
        }
    }

    ~QFileRAII() {
        if (file.isOpen()) {
            file.close();
        }
    }

    bool isOpen() const {
        return file.isOpen();
    }

    QString readAll() {
        if (file.isOpen()) {
            QTextStream in(&file);
            return in.readAll();
        }
        return QString();
    }

private:
    QFile file;
};
