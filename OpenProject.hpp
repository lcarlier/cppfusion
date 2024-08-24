#pragma once

#include <memory>

#include <QDialog>

#include "ui_OpenProject.h"

#include "ClangdClient.hpp"

namespace Ui {
class OpenProject;
}

class OpenProject : public QDialog
{
    Q_OBJECT

public:
    explicit OpenProject(QWidget *parent = nullptr);
    ClangdProject getClangdProject();

private:
    std::unique_ptr<Ui::OpenProject> ui;

    void validate();
private slots:
    void textChanged(const QString& str);
    void browseRootProjectClicked(bool checked = false);
    void browseCompileCommandsClicked(bool checked = false);
};

