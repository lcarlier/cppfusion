#include <QDebug>
#include <QObject>
#include <QMessageBox>
#include <QPlainTextEdit>

#include "MainWindow.hpp"
#include "./ui_MainWindow.h"
#include "OpenProject.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), clangdClient{nullptr}, clientDialog{nullptr}, projectModel{nullptr}, ui(new Ui::MainWindow) {
    ui->setupUi(this);

    while(ui->tabWidgetOpenFile->count() > 0)
    {
        closeTab(0);
    }

    connect(ui->actionShow_Clang_Debug, &QAction::triggered, this,
            &MainWindow::showClangDebugDialog);
    connect(ui->actionOpen_project, &QAction::triggered, this, &MainWindow::showOpenProject);
    connect(ui->treeViewProject, &QTreeView::doubleClicked, this, &MainWindow::onProjectFileDoubleClick);
    connect(ui->tabWidgetOpenFile, &QTabWidget::tabCloseRequested, this, &MainWindow::tabCloseRequested);
}


void MainWindow::showOpenProject(bool /*triggered*/)
{
    OpenProject openProject;
    auto result = openProject.exec();
    if(result == OpenProject::Accepted)
    {
        ClangdProject clangdProject = openProject.getClangdProject();
        {
            projectModel.reset(new ProjectModel{clangdProject, ui->treeViewProject});
            ui->treeViewProject->setModel(projectModel.get());
        }
        clangdClient.reset(new ClangdClient{clangdProject, this});
        clientDialog.reset(new ClangClientDialog{*clangdClient, clangdProject, this});
        clientDialog->setWindowFlags(clientDialog->windowFlags() | Qt::WindowMaximizeButtonHint | Qt::Window);
    }
}

void MainWindow::showClangDebugDialog(bool /*triggered*/)
{
    if(!clientDialog)
    {
        QMessageBox::critical(this, "Cannot open the debug dialog", "Cannot open the debug dialog\nNo project open");
        return;
    }
    clientDialog->show();
}

void MainWindow::onProjectFileDoubleClick(const QModelIndex &index) {
    if(!index.isValid())
    {
        return;
    }
    ProjectModel *model = static_cast<ProjectModel *>(ui->treeViewProject->model());
    QString filePath = model->filePath(index);
    QFileInfo fileInfo(filePath);

    if(fileInfo.isFile())
    {
        QPlainTextEdit* newEdit = new QPlainTextEdit{ui->tabWidgetOpenFile};
        {
            QFileRAII thisFile{filePath};
            newEdit->setPlainText(thisFile.readAll());
        }

        ui->tabWidgetOpenFile->addTab(newEdit, fileInfo.fileName());
    }
}

void MainWindow::tabCloseRequested(int index)
{
    closeTab(index);
}

void MainWindow::closeTab(int index)
{
    auto* widget = ui->tabWidgetOpenFile->widget(index);
    ui->tabWidgetOpenFile->removeTab(index);
    delete widget;
}

MainWindow::~MainWindow() {}
