#include <QStandardPaths>
#include <QFile>
#include <QFileDialog>
#include <QDir>

#include "OpenProject.hpp"
#include "ui_OpenProject.h"

#include "ApplicationSettings.hpp"

OpenProject::OpenProject(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OpenProject)
{
    ui->setupUi(this);

    connect(ui->lineEditProjectRoot, &QLineEdit::textChanged, this, &OpenProject::textChanged);
    connect(ui->pushButtonBrowsePorjectRoot, &QPushButton::clicked, this, &OpenProject::browseRootProjectClicked);
    connect(ui->pushButtonBrowsePathToCompileCommandJson, &QPushButton::clicked, this, &OpenProject::browseCompileCommandsClicked);

    ui->lineEditPathToClangd->setText(QStandardPaths::findExecutable("clangd"));
    validate();
}

ClangdProject OpenProject::getClangdProject()
{
    return ClangdProject{ui->lineEditProjectRoot->text(), ui->lineEditPathToCompileCommandsJson->text(), ui->lineEditPathToClangd->text()};
}

void OpenProject::validate()
{
    bool rootDirValid = false;
    {
        const QString& curText = ui->lineEditProjectRoot->text();
        QDir rootDir = ui->lineEditProjectRoot->text();
        if(!curText.isEmpty() && rootDir.exists())
        {
            rootDirValid = true;
        }
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(rootDirValid);
}

void OpenProject::textChanged(const QString& /*str*/)
{
    validate();
}

void OpenProject::browseRootProjectClicked(bool /*checked*/)
{
    ApplicationSettings appSettings;
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::Directory);
    fileDialog.setDirectory(appSettings.getLastRootPathSelected());
    if(fileDialog.exec())
    {
        const auto& selectedFiles = fileDialog.selectedFiles();
        QDir dirName{selectedFiles[0]};
        appSettings.setLastRootPathSelected(dirName);
        ui->lineEditProjectRoot->setText(dirName.absolutePath());
    }
}

void OpenProject::browseCompileCommandsClicked(bool /*checked*/)
{
    ApplicationSettings appSettings;
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setNameFilter("Compile commands JSON (compile_commands.json)");
    fileDialog.setDirectory(appSettings.getDirLastCompileCommandJsonSelected());
    if(fileDialog.exec())
    {
        const auto& selectedFiles = fileDialog.selectedFiles();
        QFileInfo fileName{selectedFiles[0]};
        appSettings.setDirLastCompileCommandJsonSelected(fileName.dir());
        ui->lineEditPathToCompileCommandsJson->setText(fileName.absoluteFilePath());
    }
}
